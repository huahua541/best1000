#ifndef __HAL_CODEC_H__
#define __HAL_CODEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "hal_aud.h"

enum HAL_CODEC_ID_T {
    HAL_CODEC_ID_0 = 0,
    HAL_CODEC_ID_NUM,
};

struct HAL_CODEC_CONFIG_T {
    enum AUD_BITS_T bits;
    enum AUD_SAMPRATE_T sample_rate;
    enum AUD_CHANNEL_NUM_T channel_num;

    uint32_t use_dma:1;
    uint32_t vol:5;

    enum AUD_IO_PATH_T io_path;

    uint32_t set_flag;
};

enum HAL_CODEC_CONFIG_FLAG_T{
    HAL_CODEC_CONFIG_NULL = 0x00,

    HAL_CODEC_CONFIG_BITS = 0x01,
    HAL_CODEC_CONFIG_SMAPLE_RATE = 0x02,
    HAL_CODEC_CONFIG_CHANNEL_NUM = 0x04,
    HAL_CODEC_CONFIG_USE_DMA = 0x08,
    HAL_CODEC_CONFIG_VOL = 0x10,
    HAL_CODEC_CONFIG_IO_PATH = 0x20,
//    HAL_CODEC_CONFIG_DUAL_MIC = 0x10,
//    HAL_CODEC_CONFIG_DUAL_DAC = 0x20,

//    HAL_CODEC_CONFIG_INPUT_PATH = 0x80,

    HAL_CODEC_CONFIG_ALL = 0xff,
};


int hal_codec_open(enum HAL_CODEC_ID_T id);
int hal_codec_close(enum HAL_CODEC_ID_T id);
int hal_codec_start_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream);
int hal_codec_stop_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream);
int hal_codec_setup_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream, struct HAL_CODEC_CONFIG_T *cfg);
int hal_codec_send(enum HAL_CODEC_ID_T id, uint8_t *value, uint32_t value_len);
uint8_t hal_codec_recv(enum HAL_CODEC_ID_T id, uint8_t *value, uint32_t value_len);
int hal_codec_sdm_gain_set(enum HAL_CODEC_ID_T id, uint32_t gain);
uint32_t hal_codec_get_dac_osr(enum HAL_CODEC_ID_T id);
uint32_t hal_codec_get_dac_up(enum HAL_CODEC_ID_T id);
uint32_t hal_codec_get_adc_down(enum HAL_CODEC_ID_T id);

#ifdef __cplusplus
}
#endif

#endif
