/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>

#include "microphone.h"

#define AUDIO_BUF_SIZE_MS 	200
#define AUDIO_BUF_SIZE 		((MICROPHONE_SAMPLE_RATE * AUDIO_BUF_SIZE_MS) / MSEC_PER_SEC)

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static struct gpio_dt_spec p_led_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static const struct device *const p_mic_dev = DEVICE_DT_GET(DT_NODELABEL(mp34dt06j));

static volatile int16_t audio_buf[AUDIO_BUF_SIZE];

int main(void)
{
	gpio_pin_configure_dt(&p_led_dev, GPIO_OUTPUT);

	gpio_pin_set_dt(&p_led_dev, 1);

	LOG_INF("DMIC example");

	microphone_init(p_mic_dev);
	microphone_start(p_mic_dev);
	microphone_fill_buffer(p_mic_dev, (int16_t *) audio_buf, AUDIO_BUF_SIZE);
	microphone_stop(p_mic_dev);

	k_sleep(K_FOREVER);

	return 0;
}

