#include "audio_proc.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/timeutil.h>
#include <stdio.h>

#include "inference.h"
#include "mfcc.h"
#include "storage.h"
#include "audio_acq.h"

#define FEATURE_SIZE (13*43)
#define FILENAME_LEN (18U)
#define SET_TIME 1

LOG_MODULE_REGISTER(audio_proc, LOG_LEVEL_DBG);

static const struct device *const p_rtc_dev = DEVICE_DT_GET(DT_NODELABEL(rtc));

static char filename[FILENAME_LEN];
static char line[STORAGE_MAX_LINE_LEN];

static K_SEM_DEFINE(proc_start_sem, 0, 1);

int audio_proc_init(void)
{
	int ret;
	struct rtc_time rtc_ts;

#if SET_TIME
	rtc_ts.tm_year = 124;
	rtc_ts.tm_mon = 5;
	rtc_ts.tm_mday = 14;
	rtc_ts.tm_wday = 2;
	rtc_ts.tm_hour = 10;
	rtc_ts.tm_min = 24;
	rtc_ts.tm_sec = 30;
	rtc_ts.tm_nsec = 0;

	ret = rtc_set_time(p_rtc_dev, &rtc_ts);
	if (ret != 0)
	{
		return ret;
	}
#endif /* SET_TIME */

	ret = rtc_get_time(p_rtc_dev, &rtc_ts);
	if (ret != 0)
	{
		return ret;
	}

	sprintf(filename, "%04d%02d%02d-%02d%02d.csv",
		rtc_ts.tm_year + 1900,
		rtc_ts.tm_mon,
		rtc_ts.tm_mday,
		rtc_ts.tm_hour,
		rtc_ts.tm_min);

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

void audio_proc_start(void)
{
	k_sem_give(&proc_start_sem);
}

void audio_proc_run(void *p1, void *p2, void *p3)
{
	int ret;
	float mfcc[FEATURE_SIZE];
	output_values_t output;
	struct rtc_time rtc_ts;
	int64_t posix_ts;

	int16_t *p_audio_buf = audio_acq_get_secondary_buf_ptr();

	for (;;)
	{
		k_sem_take(&proc_start_sem, K_FOREVER);

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

			rtc_get_time(p_rtc_dev, &rtc_ts);
			posix_ts = timeutil_timegm64(rtc_time_to_tm(&rtc_ts));

			sprintf(line, "%lld,%d,%d,%d\n",
					posix_ts,
					(int) (output.block * 100),
					(int) (output.prolongation * 100),
					(int) (output.repetition * 100));

			storage_write_line(line, filename);
		}
	}
}
