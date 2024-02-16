/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/audio/dmic.h>

#include <zephyr/logging/log.h>

#define SAMPLE_RATE 16000
#define SAMPLE_BIT_WIDTH 16
#define BYTES_PER_SAMPLE sizeof(int16_t)

#define READ_TIMEOUT_MS 1000

#define BLOCK_SIZE(_sample_rate, _number_of_channels) \
	(BYTES_PER_SAMPLE * (_sample_rate / 100) * _number_of_channels)

#define SINGLE_BLOCK_SIZE   BLOCK_SIZE(SAMPLE_RATE, 1)
#define BLOCK_COUNT      4

#define AUDIO_BUF_SIZE_MS 100
#define AUDIO_BUF_SIZE ((SAMPLE_RATE * AUDIO_BUF_SIZE_MS) / MSEC_PER_SEC)

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

K_MEM_SLAB_DEFINE_STATIC(mem_slab, SINGLE_BLOCK_SIZE, BLOCK_COUNT, 4);

static const struct gpio_dt_spec led_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static const struct device *const dmic_dev = DEVICE_DT_GET(DT_NODELABEL(mp34dt06j));

static struct pcm_stream_cfg stream = {
	.pcm_width = SAMPLE_BIT_WIDTH,
	.mem_slab  = &mem_slab,
	.pcm_rate = SAMPLE_RATE,
	.block_size = SINGLE_BLOCK_SIZE,
};

static struct dmic_cfg cfg = {
	.io = {
		/* These fields can be used to limit the PDM clock
			* configurations that the driver is allowed to use
			* to those supported by the microphone.
			*/
		.min_pdm_clk_freq = 1200000,
		.max_pdm_clk_freq = 3250000,
	},
	.streams = &stream,
	.channel = {
		.req_num_streams = 1,
		.req_num_chan = 1,
	},
};

static volatile int16_t audio_buf[AUDIO_BUF_SIZE];

int main(void)
{
	int ret;

	gpio_pin_configure_dt(&led_dev, GPIO_OUTPUT);

	gpio_pin_set_dt(&led_dev, 1);

	LOG_INF("DMIC example");

	if (!device_is_ready(dmic_dev)) {
		LOG_ERR("%s is not ready", dmic_dev->name);
		return 0;
	}

	ret = dmic_configure(dmic_dev, &cfg);

	if (ret < 0)
	{
		LOG_ERR("DMIC configure error %d", ret);
		return 0;
	}

	k_msleep(500);

	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);

	if (ret < 0)
	{
		LOG_ERR("DMIC trigger error %d", ret);
		return 0;
	}

	size_t sample_count;

	for (sample_count = 0; sample_count < AUDIO_BUF_SIZE;)
	{
		void *pcm_buffer;
		size_t size; 

		ret = dmic_read(dmic_dev, 0, &pcm_buffer, &size, READ_TIMEOUT_MS);

		if (ret < 0)
		{
			LOG_ERR("DMIC read failed with err %d", ret);
			return 0;
		}

		memcpy((void *) &audio_buf[sample_count], pcm_buffer, size);

		sample_count += size / sizeof(int16_t);

		k_mem_slab_free(&mem_slab, pcm_buffer);
	}

	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
	if (ret < 0) {
		LOG_ERR("STOP trigger failed: %d", ret);
		return 0;
	}

	return 0;
}

