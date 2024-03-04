#ifndef _INFERENCE_H_
#define _INFERENCE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct output_values 
{
    float block;
    float prolongation;
    float word_rep;
    float sound_rep;
} output_values_t;

int inference_setup(void);
int inference_run(output_values_t *output_val);

#ifdef __cplusplus
}
#endif

#endif /* _INFERENCE_H_ */
