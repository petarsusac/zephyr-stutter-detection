#include "audio_proc.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "inference.h"
#include "mfcc.h"
#include "storage.h"
#include "audio_acq.h"

#define FEATURE_SIZE (13*43)

LOG_MODULE_REGISTER(audio_proc, LOG_LEVEL_DBG);

static const char* p_filename = "20240502-1134.txt";
static char p_line[STORAGE_MAX_LINE_LEN];

int audio_proc_init(void)
{
	int ret;

	ret = storage_init();
	// Temporarily commented out to be able to debug without using the SD card
	// if (ret != 0)
	// {
	//  return ret;
	// }

	ret = mfcc_init();
	if (ret != 0)
	{
		return ret;
	}

	ret = inference_setup();
	if (ret != 0)
	{
		return ret;
	}

	return 0;
}

void audio_proc_run(void *p1, void *p2, void *p3)
{
	int ret;
	float mfcc[FEATURE_SIZE];
	output_values_t output;

	struct k_sem *p_sem = (struct k_sem *) p1;

	int16_t *p_audio_buf = audio_acq_get_secondary_buf_ptr();

	for (;;)
	{
		k_sem_take(p_sem, K_FOREVER);

		int64_t start_ms = k_uptime_get();

		ret = mfcc_run(p_audio_buf, mfcc, AUDIO_ACQ_SCND_BUF_SIZE);

		if (0 == ret)
		{
			ret = inference_run(mfcc, FEATURE_SIZE, &output);
		}

		if (0 == ret)
		{
			LOG_INF("Block: %d%%", (int) (output.block * 100));
			LOG_INF("Prolongation: %d%%", (int) (output.prolongation * 100));
			LOG_INF("Repetition: %d%%", (int) (output.repetition * 100));

			int64_t diff = k_uptime_delta(&start_ms);

			LOG_DBG("Elapsed: %lld ms", diff);

			sprintf(p_line, 
					"yyyy-mm-dd-hh-mm-ss,%d,%d,%d\n",
					(int) (output.block * 100),
					(int) (output.prolongation * 100),
					(int) (output.repetition * 100));

			storage_write_line(p_line, p_filename);
		}
	}
}
