#ifndef __HAL_I2S_H__
#define __HAL_I2S_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "hal_aud.h"

enum HAL_I2S_ID_T {
	HAL_I2S_ID_0 = 0,
	HAL_I2S_ID_NUM,
};

struct HAL_I2S_CONFIG_T {
    uint32_t bits;
    uint32_t channel_num;
    uint32_t sample_rate;

    uint32_t use_dma;
};

int hal_i2s_open(enum HAL_I2S_ID_T id);
int hal_i2s_close(enum HAL_I2S_ID_T id);
int hal_i2s_start_stream(enum HAL_I2S_ID_T id, enum AUD_STREAM_T stream);
int hal_i2s_stop_stream(enum HAL_I2S_ID_T id, enum AUD_STREAM_T stream);
int hal_i2s_setup_stream(enum HAL_I2S_ID_T id, enum AUD_STREAM_T stream, struct HAL_I2S_CONFIG_T *cfg);
int hal_i2s_send(enum HAL_I2S_ID_T id, uint8_t *value, uint32_t value_len);
uint8_t hal_i2s_recv(enum HAL_I2S_ID_T id, uint8_t *value, uint32_t value_len);

#ifdef __cplusplus
}
#endif

#endif
