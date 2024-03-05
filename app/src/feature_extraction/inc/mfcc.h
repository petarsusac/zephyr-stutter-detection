#ifndef _MFCC_H_
#define _MFCC_H_

#include "arm_math.h"
#include <stdint.h>

int mfcc_init(void);
int mfcc_run(int16_t *p_in_signal, float *p_out_mfcc, uint32_t signal_len);

#endif // _MFCC_H_
