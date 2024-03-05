#include "inference.h"

#include "model.hpp"

#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(inference, LOG_LEVEL_DBG);

namespace {
	const tflite::Model *model = nullptr;
	tflite::MicroInterpreter *interpreter = nullptr;
	TfLiteTensor *input = nullptr;

	constexpr int outputs_len = 4;
	TfLiteTensor *outputs[outputs_len] = {nullptr};

	constexpr int kTensorArenaSize = 100 * 1024;
	uint8_t tensor_arena[kTensorArenaSize];
}  /* namespace */

int inference_setup(void)
{
    model = tflite::GetModel(g_model);
	if (model->version() != TFLITE_SCHEMA_VERSION) {
		LOG_ERR("Model provided is schema version %d not equal "
					"to supported version %d.",
					model->version(), TFLITE_SCHEMA_VERSION);
		return -1;
	}

    static tflite::MicroMutableOpResolver <9> resolver;
    resolver.AddReshape();
    resolver.AddConv2D();
    resolver.AddMul();
    resolver.AddAdd();
    resolver.AddMaxPool2D();
    resolver.AddUnidirectionalSequenceLSTM();
    resolver.AddStridedSlice();
    resolver.AddFullyConnected();
    resolver.AddLogistic();

    static tflite::MicroInterpreter static_interpreter(
		model, resolver, tensor_arena, kTensorArenaSize);
	interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
	if (allocate_status != kTfLiteOk) {
		LOG_ERR("AllocateTensors() failed");
		return -1;
	}

    input = interpreter->input(0);
	for (int i = 0; i < outputs_len; i++)
	{
		outputs[i] = interpreter->output(i);
	}

    return 0;
}

int inference_run(float *p_input, size_t input_len, output_values_t *p_output_val)
{
	for (size_t i = 0; i < input_len; i++)
	{
		input->data.int8[i] = p_input[i] / input->params.scale + input->params.zero_point;
	}

	TfLiteStatus invoke_status = interpreter->Invoke();
	if (invoke_status != kTfLiteOk) {
		return -1;
	}

	p_output_val->block = (outputs[0]->data.int8[0] - outputs[0]->params.zero_point) * outputs[0]->params.scale;
	p_output_val->prolongation = (outputs[1]->data.int8[0] - outputs[1]->params.zero_point) * outputs[1]->params.scale;
	p_output_val->word_rep = (outputs[2]->data.int8[0] - outputs[2]->params.zero_point) * outputs[2]->params.scale;
	p_output_val->sound_rep = (outputs[3]->data.int8[0] - outputs[3]->params.zero_point) * outputs[3]->params.scale;

	return 0;
}
