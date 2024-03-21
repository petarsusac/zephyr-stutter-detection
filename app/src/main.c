/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/logging/log.h>

#include "microphone.h"
#include "inference.h"
#include "mfcc.h"

#define AUDIO_BUF_SIZE_MS 	6000
#define AUDIO_BUF_SIZE 		((MICROPHONE_SAMPLE_RATE * AUDIO_BUF_SIZE_MS) / MSEC_PER_SEC)
#define FEATURE_SIZE 		(13*43)

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static int print_buffer(const struct device *p_uart, int16_t *buf, size_t len);
static void inference_thread_run(void *p1, void *p2, void *p3);

static struct gpio_dt_spec p_led_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static const struct device *const p_mic_dev = DEVICE_DT_GET(DT_NODELABEL(mp34dt06j));
static const struct device *const p_uart_dev = DEVICE_DT_GET(DT_NODELABEL(usart2));

static int16_t audio_buf[AUDIO_BUF_SIZE];

K_THREAD_DEFINE(inference_thread, 8*1024, inference_thread_run, NULL, NULL, NULL, 1, 0, 0);
K_SEM_DEFINE(inference_run_sem, 0, 1);

int main(void)
{
	gpio_pin_configure_dt(&p_led_dev, GPIO_OUTPUT);

	gpio_pin_set_dt(&p_led_dev, 1);

	LOG_INF("DMIC example");

	microphone_init(p_mic_dev);
	microphone_start(p_mic_dev);

	for(;;)
	{
		microphone_fill_buffer(p_mic_dev, &audio_buf[0], AUDIO_BUF_SIZE / 2);
		k_sem_give(&inference_run_sem);
		microphone_fill_buffer(p_mic_dev, &audio_buf[AUDIO_BUF_SIZE / 2], AUDIO_BUF_SIZE / 2);
		k_sem_give(&inference_run_sem);
	}

	print_buffer(p_uart_dev, audio_buf, AUDIO_BUF_SIZE);

	k_sleep(K_FOREVER);

	return 0;
}

static int print_buffer(const struct device *p_uart, int16_t *buf, size_t len)
{
	uint8_t pcm_l, pcm_h;
	
	if (!device_is_ready(p_uart))
	{
		LOG_ERR("UART not ready");
		return -1;
	}

	for (size_t i = 0; i < len; i++)
	{
		pcm_l = (uint8_t) (buf[i] & 0xFF);
		pcm_h = (uint8_t) ((buf[i] >> 8) & 0xFF);

		uart_poll_out(p_uart, pcm_h); 
		uart_poll_out(p_uart, pcm_l);
	}

	return 0;
}

static void inference_thread_run(void *p1, void *p2, void *p3)
{
	int ret;
	float mfcc[FEATURE_SIZE];
	output_values_t output;
	size_t start_idx = 0;

	ret = mfcc_init();
	if (ret != 0)
	{
		return;
	}
	
	ret = inference_setup();
	if (ret != 0)
	{
		return;
	}

	for (;;)
	{
		k_sem_take(&inference_run_sem, K_FOREVER);

		int64_t start_ms = k_uptime_get();

		mfcc_run(&audio_buf[start_idx], mfcc, AUDIO_BUF_SIZE / 2);

		ret = inference_run(mfcc, FEATURE_SIZE, &output);

		if (ret != 0)
		{
			LOG_ERR("Inference failed");
		}
		else
		{
			LOG_INF("Block: %d%%", (int) (output.block * 100));
			LOG_INF("Prolongation: %d%%", (int) (output.prolongation * 100));
			LOG_INF("Repetition: %d%%", (int) (output.repetition * 100));
		}

		int64_t diff = k_uptime_delta(&start_ms);

		LOG_INF("Elapsed: %lld ms", diff);

		start_idx = (0 == start_idx) ? (AUDIO_BUF_SIZE / 2) : 0;
	}

	k_sleep(K_FOREVER);
}

