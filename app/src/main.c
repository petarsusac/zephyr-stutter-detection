/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>

#include "audio_proc.h"
#include "audio_acq.h"
#include "microphone.h"
#include "inference.h"
#include "mfcc.h"
#include "storage.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static struct gpio_dt_spec p_led_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static const struct device *const p_mic_dev = DEVICE_DT_GET(DT_NODELABEL(mp34dt06j));

K_SEM_DEFINE(proc_run_sem, 0, 1);

K_THREAD_DEFINE(audio_acq_thread,
				AUDIO_ACQ_STACK_SIZE, 
				audio_acq_run, 
				&proc_run_sem, NULL, NULL, AUDIO_ACQ_PRIO, 0, 100);

K_THREAD_DEFINE(audio_proc_thread,
				AUDIO_PROC_STACK_SIZE, 
				audio_proc_run, 
				&proc_run_sem, NULL, NULL, AUDIO_PROC_PRIO, 0, 100);

int main(void)
{
	int ret;
	
	LOG_INF("Application start");

	gpio_pin_configure_dt(&p_led_dev, GPIO_OUTPUT);

	gpio_pin_set_dt(&p_led_dev, 1);

	ret = storage_init();
	if (ret != 0)
	{
		return -1;
	}

	ret = mfcc_init();
	if (ret != 0)
	{
		return -1;
	}

	ret = inference_setup();
	if (ret != 0)
	{
		return -1;
	}

	ret = microphone_init(p_mic_dev);
	if (ret != 0)
	{
		return -1;
	}

	k_sleep(K_FOREVER);

	return 0;
}
