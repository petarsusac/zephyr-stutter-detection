#ifndef _INFERENCE_H_
#define _INFERENCE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct output_values 
{
    float block;
    float prolongation;
    float repetition;
} output_values_t;

int inference_setup(void);
int inference_run(float *p_input, size_t input_len, output_values_t *p_output_val);

#ifdef __cplusplus
}
#endif

#endif /* _INFERENCE_H_ */
