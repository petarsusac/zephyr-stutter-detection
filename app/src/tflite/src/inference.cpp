#include "inference.h"

#include "model.hpp"

#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(inference, LOG_LEVEL_DBG);

extern const float input_data[13][47];

namespace {
	const tflite::Model *model = nullptr;
	tflite::MicroInterpreter *interpreter = nullptr;
	TfLiteTensor *input = nullptr;
	TfLiteTensor *output = nullptr;

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
	output = interpreter->output(0);

    return 0;
}

float inference_run(void)
{
	for (int i = 0; i < 13; i++)
	{
		for (int j = 0; j < 47; j++)
		{
			input->data.int8[i*47 + j] = input_data[i][j] / input->params.scale + input->params.zero_point;
		}
	}

	TfLiteStatus invoke_status = interpreter->Invoke();
	if (invoke_status != kTfLiteOk) {
		return -1;
	}

	int8_t output_quantized = output->data.int8[0];
	return (output_quantized - output->params.zero_point) * output->params.scale;
}
