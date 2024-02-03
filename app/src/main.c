/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

static struct gpio_dt_spec led_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

int main(void)
{

	gpio_pin_configure_dt(&led_dev, GPIO_OUTPUT);

	gpio_pin_set_dt(&led_dev, 0);

	for (;;)
	{
		gpio_pin_toggle_dt(&led_dev);
		k_msleep(1000);
	}

	return 0;
}

