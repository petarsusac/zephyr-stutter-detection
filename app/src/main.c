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
#include "bt_ncp.h"
#include "microphone.h"

#define BT_CONN_TIMEOUT_MS (60000U)

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

K_THREAD_DEFINE(audio_acq_thread,
				AUDIO_ACQ_STACK_SIZE, 
				audio_acq_run, 
				NULL, NULL, NULL, 
				AUDIO_ACQ_PRIO, 0, 0);

K_THREAD_DEFINE(audio_proc_thread,
				AUDIO_PROC_STACK_SIZE, 
				audio_proc_run, 
				NULL, NULL, NULL, 
				AUDIO_PROC_PRIO, 0, 0);

K_THREAD_DEFINE(bt_ncp_thread,
				BT_NCP_STACK_SIZE,
				bt_ncp_run,
				NULL, NULL, NULL,
				BT_NCP_PRIO, 0, 0);

int main(void)
{
	int ret;
	
	LOG_INF("Application start");
	
	ret = audio_acq_init();
	if (ret != 0)
	{
		return -1;
	}

	ret = audio_proc_init();
	if (ret != 0)
	{
		return -1;
	}

	ret = bt_ncp_init();
	if (ret != 0)
	{
		return -1;
	}

	bt_ncp_wait_for_connection(BT_CONN_TIMEOUT_MS);

	audio_acq_start();

	return 0;
}
