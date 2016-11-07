#ifndef CODEC_BEST1000_H
#define CODEC_BEST1000_H

#include "plat_types.h"
#include "hal_codec.h"
#include "hal_aud.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t codec_best1000_open(void);
uint32_t codec_best1000_stream_open(enum AUD_STREAM_T stream);
uint32_t codec_best1000_stream_setup(enum AUD_STREAM_T stream, struct HAL_CODEC_CONFIG_T *cfg);
uint32_t codec_best1000_stream_start(enum AUD_STREAM_T stream);
uint32_t codec_best1000_stream_stop(enum AUD_STREAM_T stream);
uint32_t codec_best1000_stream_close(enum AUD_STREAM_T stream);
uint32_t codec_best1000_close(void);
#ifdef __CODEC_ASYNC_CLOSE__
uint32_t codec_best1000_real_close(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CODEC_H */
