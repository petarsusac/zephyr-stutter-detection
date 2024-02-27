#include "microphone.h"

#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/logging/log.h>

#define PCM_SAMPLE_WIDTH_BITS   16
#define BYTES_PER_SAMPLE        (sizeof(uint16_t))
#define MEM_SLAB_BLOCK_COUNT    4

#define BLOCK_SIZE_BYTES        (BYTES_PER_SAMPLE * (MICROPHONE_SAMPLE_RATE * MICROPHONE_BLOCK_SIZE_MS) / MSEC_PER_SEC)

#define PDM_CLOCK_FREQ_MIN      1200000
#define PDM_CLOCK_FREQ_MAX      3250000
#define READ_TIMEOUT_MS         2000

LOG_MODULE_REGISTER(microphone, LOG_LEVEL_DBG);

K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE_BYTES, MEM_SLAB_BLOCK_COUNT, 4);

int microphone_init(const struct device *p_mic_dev)
{
    int ret;

    struct pcm_stream_cfg stream = {
        .pcm_width = PCM_SAMPLE_WIDTH_BITS,
        .mem_slab  = &mem_slab,
        .pcm_rate = MICROPHONE_SAMPLE_RATE,
        .block_size = BLOCK_SIZE_BYTES,
    };

    struct dmic_cfg cfg = {
        .io = {
            .min_pdm_clk_freq = PDM_CLOCK_FREQ_MIN,
            .max_pdm_clk_freq = PDM_CLOCK_FREQ_MAX,
        },
        .streams = &stream,
        .channel = {
            .req_num_streams = 1,
            .req_num_chan = 1,
        },
    };

    if (!device_is_ready(p_mic_dev)) {
		LOG_ERR("%s is not ready", p_mic_dev->name);
		return -EIO;
	}

    ret = dmic_configure(p_mic_dev, &cfg);
    if (ret < 0)
    {
        LOG_ERR("Failed to configure mic, err %d", ret);
        return ret;
    }

    return 0;
}

int microphone_start(const struct device *p_mic_dev)
{
    int ret;

    ret = dmic_trigger(p_mic_dev, DMIC_TRIGGER_START);

    if (ret < 0)
	{
		LOG_ERR("DMIC trigger error %d", ret);
	}

    return ret;
}

int microphone_fill_buffer(const struct device *p_mic_dev, int16_t *p_buf, size_t len)
{
    int ret;
    size_t sample_count;

    for (sample_count = 0; sample_count < len;)
	{
		void *p_block;
		size_t size; 

		ret = dmic_read(p_mic_dev, 0, &p_block, &size, READ_TIMEOUT_MS);

		if (ret < 0)
		{
			LOG_ERR("DMIC read failed with err %d", ret);
			return ret;
		}

		memcpy((void *) &p_buf[sample_count], p_block, size);

        // Size is in bytes, divide it by 2 to get the sample count
		sample_count += size / sizeof(int16_t);

		k_mem_slab_free(&mem_slab, p_block);
	}

    return 0;
}

int microphone_stop(const struct device *p_mic_dev)
{
    int ret;

    ret = dmic_trigger(p_mic_dev, DMIC_TRIGGER_STOP);
	
    if (ret < 0) {
		LOG_ERR("STOP trigger failed: %d", ret);
	}

    return ret;
}
