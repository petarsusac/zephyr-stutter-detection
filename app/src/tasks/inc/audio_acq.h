#ifndef _AUDIO_ACQ_H_
#define _AUDIO_ACQ_H_

#include "microphone.h"

#define AUDIO_ACQ_PRIM_BUF_SIZE_MS 1000
#define AUDIO_ACQ_PRIM_BUF_SIZE ((MICROPHONE_SAMPLE_RATE * AUDIO_ACQ_PRIM_BUF_SIZE_MS) / MSEC_PER_SEC)

#define AUDIO_ACQ_SCND_BUF_SIZE_MS 3000
#define AUDIO_ACQ_SCND_BUF_SIZE	((MICROPHONE_SAMPLE_RATE * AUDIO_ACQ_SCND_BUF_SIZE_MS) / MSEC_PER_SEC)

#define AUDIO_ACQ_STACK_SIZE (1024U)
#define AUDIO_ACQ_PRIO (1U)

int audio_acq_init(void);
void audio_acq_start(void);
void audio_acq_run(void *p1, void *p2, void *p3);
int16_t *audio_acq_get_primary_buf_ptr(void);
int16_t *audio_acq_get_secondary_buf_ptr(void);

#endif /* _AUDIO_ACQ_H_ */