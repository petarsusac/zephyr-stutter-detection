#include "inference.h"

#include "model_block.hpp"
#include "model_repetition.hpp"
#include "model_prolongation.hpp"

#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(inference, LOG_LEVEL_DBG);

namespace {
	const int num_models = 3;
	typedef struct model_data
	{
		const unsigned char *model_buf;
		const tflite::Model *tflite_model;
		tflite::MicroInterpreter *interpreter;
		TfLiteTensor *input;
		TfLiteTensor *output;
		float train_mean;
		float train_std;
		const int tensor_arena_size;
		uint8_t *tensor_arena;
	} model_data_t;

	uint8_t tensor_arena_0[25*1024];
	uint8_t tensor_arena_1[25*1024];
	uint8_t tensor_arena_2[25*1024];
	
	model_data_t models[num_models] = {
		{
			.model_buf = model_block,
			.train_mean = -21.868359f,
			.train_std = 84.423965f,
			.tensor_arena_size = 25*1024,
			.tensor_arena = tensor_arena_0,
		},
		{
			.model_buf = model_repetition,
			.train_mean = -21.2515f,
			.train_std = 82.97f,
			.tensor_arena_size = 25*1024,
			.tensor_arena = tensor_arena_1,
		},
		{
			.model_buf = model_prolongation,
			.train_mean = -21.5854f,
			.train_std = 83.639854f,
			.tensor_arena_size = 25*1024,
			.tensor_arena = tensor_arena_2,
		},
	};
}  /* namespace */

int inference_setup(void)
{
	static tflite::MicroMutableOpResolver <10> resolver;
	resolver.AddReshape();
	resolver.AddTranspose();
	resolver.AddConv2D();
	resolver.AddMul();
	resolver.AddAdd();
	resolver.AddMaxPool2D();
	resolver.AddUnidirectionalSequenceLSTM();
	resolver.AddStridedSlice();
	resolver.AddFullyConnected();
	resolver.AddLogistic();

	for (int i = 0; i < num_models; i++)
	{
		models[i].tflite_model = tflite::GetModel(models[i].model_buf);
		if (models[i].tflite_model->version() != TFLITE_SCHEMA_VERSION) 
		{
			LOG_ERR("Model provided is schema version %d not equal to supported version %d.", 
				models[i].tflite_model->version(), TFLITE_SCHEMA_VERSION);
			return -1;
		}

		// hack!!
		switch (i)
		{
			case 0:
				static tflite::MicroInterpreter interpreter0(
					models[i].tflite_model, resolver, models[i].tensor_arena, models[i].tensor_arena_size);
				models[i].interpreter = &interpreter0;
			break;

			case 1:
				static tflite::MicroInterpreter interpreter1(
					models[i].tflite_model, resolver, models[i].tensor_arena, models[i].tensor_arena_size);
				models[i].interpreter = &interpreter1;
			break;

			case 2:
				static tflite::MicroInterpreter interpreter2(
					models[i].tflite_model, resolver, models[i].tensor_arena, models[i].tensor_arena_size);
				models[i].interpreter = &interpreter2;
			break;

			default:
			break;
		}

		TfLiteStatus allocate_status = models[i].interpreter->AllocateTensors();
		
		if (allocate_status != kTfLiteOk) 
		{
			LOG_ERR("AllocateTensors() failed for model %d", i);
			return -1;
		}

		LOG_DBG("Allocated bytes: %u", models[i].interpreter->arena_used_bytes());

		models[i].input = models[i].interpreter->input(0);
		models[i].output = models[i].interpreter->output(0);
	
	}

    return 0;
}

int inference_run(float *p_input, size_t input_len, output_values_t *p_output_val)
{
	for (int i = 0; i < num_models; i++)
	{
		float model_input_buf[input_len];
		memcpy(model_input_buf, p_input, input_len * sizeof(float));

		for (size_t j = 0; j < input_len; j++)
		{
			// Normalization
			model_input_buf[j] = (model_input_buf[j] - models[i].train_mean) / models[i].train_std;
			// Quantization
			models[i].input->data.int8[j] = model_input_buf[j] / models[i].input->params.scale + models[i].input->params.zero_point;
		}

		TfLiteStatus invoke_status = models[i].interpreter->Invoke();
		if (invoke_status != kTfLiteOk) {
			return -1;
		}
	}

	p_output_val->block = (models[0].output->data.int8[0] - models[0].output->params.zero_point) * models[0].output->params.scale;
	p_output_val->repetition = (models[1].output->data.int8[0] - models[1].output->params.zero_point) * models[1].output->params.scale;
	p_output_val->prolongation = (models[2].output->data.int8[0] - models[2].output->params.zero_point) * models[2].output->params.scale;
	
	return 0;
}
