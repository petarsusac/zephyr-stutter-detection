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

#define AUDIO_BUF_SIZE_MS 	6000
#define AUDIO_BUF_SIZE 		((MICROPHONE_SAMPLE_RATE * AUDIO_BUF_SIZE_MS) / MSEC_PER_SEC)

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static int print_buffer(const struct device *p_uart, int16_t *buf, size_t len);

static struct gpio_dt_spec p_led_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static const struct device *const p_mic_dev = DEVICE_DT_GET(DT_NODELABEL(mp34dt06j));
static const struct device *const p_uart_dev = DEVICE_DT_GET(DT_NODELABEL(usart2));

static int16_t audio_buf[AUDIO_BUF_SIZE];

int main(void)
{
	gpio_pin_configure_dt(&p_led_dev, GPIO_OUTPUT);

	gpio_pin_set_dt(&p_led_dev, 1);

	LOG_INF("DMIC example");

	microphone_init(p_mic_dev);
	microphone_start(p_mic_dev);
	microphone_fill_buffer(p_mic_dev, &audio_buf[0], AUDIO_BUF_SIZE / 2);
	microphone_fill_buffer(p_mic_dev, &audio_buf[AUDIO_BUF_SIZE / 2], AUDIO_BUF_SIZE / 2);
	microphone_stop(p_mic_dev);

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

