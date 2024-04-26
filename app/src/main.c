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

#define AUDIO_PROC_BUF_SIZE_MS 	3000
#define AUDIO_PROC_BUF_SIZE 	((MICROPHONE_SAMPLE_RATE * AUDIO_PROC_BUF_SIZE_MS) / MSEC_PER_SEC)

#define AUDIO_ACQ_BUF_SIZE_MS 	1000
#define AUDIO_ACQ_BUF_SIZE		((MICROPHONE_SAMPLE_RATE * AUDIO_ACQ_BUF_SIZE_MS) / MSEC_PER_SEC)

#define FEATURE_SIZE 			(13*43)

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static int print_buffer(const struct device *p_uart, int16_t *buf, size_t len);
static void inference_thread_run(void *p1, void *p2, void *p3);

static struct gpio_dt_spec p_led_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static const struct device *const p_mic_dev = DEVICE_DT_GET(DT_NODELABEL(mp34dt06j));
static const struct device *const p_uart_dev = DEVICE_DT_GET(DT_NODELABEL(usart2));

static int16_t __dtcm_bss_section audio_proc_buf[AUDIO_PROC_BUF_SIZE];
static int16_t __dtcm_bss_section audio_acq_buf[AUDIO_ACQ_BUF_SIZE];

K_THREAD_DEFINE(inference_thread, 8*1024, inference_thread_run, NULL, NULL, NULL, 1, 0, 0);
K_SEM_DEFINE(inference_run_sem, 0, 1);

int main(void)
{
	size_t audio_proc_buf_index = 0;

	gpio_pin_configure_dt(&p_led_dev, GPIO_OUTPUT);

	gpio_pin_set_dt(&p_led_dev, 1);

	LOG_INF("DMIC example");

	microphone_init(p_mic_dev);
	microphone_start(p_mic_dev);

	// Skip the first second due to a glitch on microphone startup
	microphone_fill_buffer(p_mic_dev, audio_acq_buf, AUDIO_ACQ_BUF_SIZE);

	for(;;)
	{
		microphone_fill_buffer(p_mic_dev, audio_acq_buf, AUDIO_ACQ_BUF_SIZE);
		memcpy(&audio_proc_buf[audio_proc_buf_index], audio_acq_buf, AUDIO_ACQ_BUF_SIZE * sizeof(int16_t));
		audio_proc_buf_index += AUDIO_ACQ_BUF_SIZE;

		if (audio_proc_buf_index >= AUDIO_PROC_BUF_SIZE)
		{
			audio_proc_buf_index = 0;
			k_sem_give(&inference_run_sem);
		}
	}

	print_buffer(p_uart_dev, audio_proc_buf, AUDIO_PROC_BUF_SIZE);

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

		mfcc_run(audio_proc_buf, mfcc, AUDIO_PROC_BUF_SIZE);

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
	}

	k_sleep(K_FOREVER);
}

