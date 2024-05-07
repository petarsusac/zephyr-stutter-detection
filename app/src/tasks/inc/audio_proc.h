#ifndef _AUDIO_PROC_H_
#define _AUDIO_PROC_H_

#define AUDIO_PROC_STACK_SIZE (8*1024U)
#define AUDIO_PROC_PRIO (2U)

int audio_proc_init(void);
void audio_proc_start(void);
void audio_proc_run(void *p1, void *p2, void *p3);

#endif /* _AUDIO_PROC_H_ */