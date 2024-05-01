#include "audio_acq.h"

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(audio_acq, LOG_LEVEL_DBG);

static int16_t __dtcm_bss_section prim_buf[AUDIO_ACQ_PRIM_BUF_SIZE];
static int16_t __dtcm_bss_section scnd_buf[AUDIO_ACQ_SCND_BUF_SIZE];

static const struct device *const p_mic_dev = DEVICE_DT_GET(DT_NODELABEL(mp34dt06j));
static const struct device *const p_uart_dev = DEVICE_DT_GET(DT_NODELABEL(usart2));

static int print_buffer(const struct device *p_uart, int16_t *buf, size_t len);

void audio_acq_run(void *p1, void *p2, void *p3)
{
	size_t scnd_buf_index = 0;

    struct k_sem *p_sem = (struct k_sem *) p1;

    microphone_start(p_mic_dev);

	// Skip the first second due to a glitch on microphone startup
	microphone_fill_buffer(p_mic_dev, prim_buf, AUDIO_ACQ_PRIM_BUF_SIZE);

	for(;;)
	{
		microphone_fill_buffer(p_mic_dev, prim_buf, AUDIO_ACQ_PRIM_BUF_SIZE);
		memcpy(&scnd_buf[scnd_buf_index], prim_buf, AUDIO_ACQ_PRIM_BUF_SIZE * sizeof(int16_t));
		scnd_buf_index += AUDIO_ACQ_PRIM_BUF_SIZE;

		if (scnd_buf_index >= AUDIO_ACQ_SCND_BUF_SIZE)
		{
			scnd_buf_index = 0;
			k_sem_give(p_sem);
		}
	}

    print_buffer(p_uart_dev, scnd_buf, AUDIO_ACQ_SCND_BUF_SIZE);
}

int16_t *audio_acq_get_primary_buf_ptr(void)
{
    return prim_buf;
}

int16_t *audio_acq_get_secondary_buf_ptr(void)
{
    return scnd_buf;
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

