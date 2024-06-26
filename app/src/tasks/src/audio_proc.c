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
#include "bt_ncp.h"

#define FEATURE_SIZE (13*43)
#define FILENAME_INF_RES_LEN (18U)
#define FILENAME_BIOMED_LEN (25U)

LOG_MODULE_REGISTER(audio_proc, CONFIG_APP_LOG_LEVEL);

#ifdef CONFIG_EXTERNAL_RTC
static const struct device *const p_rtc_dev = DEVICE_DT_GET(DT_NODELABEL(rtc_ext));
#else
static const struct device *const p_rtc_dev = DEVICE_DT_GET(DT_NODELABEL(rtc));
#endif /* CONFIG_EXTERNAL_RTC */

static char filename_inf_res[FILENAME_INF_RES_LEN];
static char filename_biomed[FILENAME_BIOMED_LEN];
static char line[STORAGE_MAX_LINE_LEN];

static K_SEM_DEFINE(proc_start_sem, 0, 1);

int audio_proc_init(void)
{
	int ret;
	struct rtc_time rtc_ts;

#ifdef RTC_SET_TIME
	rtc_ts.tm_year = RTC_YEAR - 1900;
	rtc_ts.tm_mon = RTC_MON;
	rtc_ts.tm_mday = RTC_MDAY;
	rtc_ts.tm_wday = RTC_WDAY;
	rtc_ts.tm_hour = RTC_HOUR;
	rtc_ts.tm_min = RTC_MIN;
	rtc_ts.tm_sec = RTC_SEC;
	rtc_ts.tm_nsec = 0;

	ret = rtc_set_time(p_rtc_dev, &rtc_ts);
	if (ret != 0)
	{
		return ret;
	}
#endif /* RTC_SET_TIME */

	ret = rtc_get_time(p_rtc_dev, &rtc_ts);
	if (ret != 0)
	{
		return ret;
	}

	snprintf(filename_inf_res, 
		FILENAME_INF_RES_LEN, 
		"%04hu%02hu%02hu-%02hu%02hu.csv",
		rtc_ts.tm_year + 1900,
		rtc_ts.tm_mon,
		rtc_ts.tm_mday,
		rtc_ts.tm_hour,
		rtc_ts.tm_min);

	snprintf(filename_biomed, 
		FILENAME_BIOMED_LEN, 
		"%04hu%02hu%02hu-%02hu%02hu_biomed.csv",
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
	bt_ncp_ts_msg_t biomed_entry;

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
		else
		{
			output.block = 0.0f;
			output.prolongation = 0.0f;
			output.repetition = 0.0f;
		}

		if (0 == ret)
		{
			LOG_INF("Block: %d%%", (int) (output.block * 100));
			LOG_INF("Prolongation: %d%%", (int) (output.prolongation * 100));
			LOG_INF("Repetition: %d%%", (int) (output.repetition * 100));

			int64_t diff = k_uptime_delta(&start_ms);

			LOG_DBG("Elapsed: %lld ms", diff);

			// Write inference results to file
			rtc_get_time(p_rtc_dev, &rtc_ts);
			posix_ts = timeutil_timegm64(rtc_time_to_tm(&rtc_ts));

			sprintf(line, "%lld,%d,%d,%d\n",
					posix_ts,
					(int) (output.block * 100),
					(int) (output.prolongation * 100),
					(int) (output.repetition * 100));

			storage_write_line(line, filename_inf_res);
		}

		// Write biomedical data to file
		do {
			ret = bt_ncp_get_timestamped_msg(&biomed_entry);
			if (0 == ret)
			{
				sprintf(line, "%lld,%u,%u,%u,%u\n",
						biomed_entry.timestamp,
						biomed_entry.msg.hr,
						biomed_entry.msg.rmssd,
						biomed_entry.msg.ppg_amplitude,
						biomed_entry.msg.epc);

				storage_write_line(line, filename_biomed);
			}
		} while (0 == ret);
	}
}
