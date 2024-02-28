#ifndef _MICROPHONE_H_
#define _MICROPHONE_H_

#include <zephyr/device.h>
#include <stdint.h>

#define MICROPHONE_SAMPLE_RATE 8000
#define MICROPHONE_BLOCK_SIZE_MS 4

int microphone_init(const struct device *p_mic_dev);

int microphone_start(const struct device *p_mic_dev);

int microphone_fill_buffer(const struct device *p_mic_dev, int16_t *p_buf, size_t len);

int microphone_stop(const struct device *p_mic_dev);

#endif /* _MICROPHONE_H_ */