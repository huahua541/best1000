/* rbpcmbuf source */
/* pcmbuf management & af control & mixer */

#include "mbed.h"
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "rtos.h"
#include <ctype.h>
#include <unistd.h>
#include "SDFileSystem.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "audioflinger.h"
#include "cqueue.h"
#include "app_audio.h"

#include "eq.h"
#include "pga.h"
#include "metadata.h"
#include "dsp_core.h"
#include "codecs.h"
#include "codeclib.h"
#include "compressor.h"
#include "channel_mode.h"
#include "audiohw.h"
#include "codec_platform.h"
#include "metadata_parsers.h"

#include "hal_overlay.h"
#include "app_overlay.h"
#include "rbplay.h"
#include "rbpcmbuf.h"

/* media buffer is buffering buffer between codec and audioflinger */
/* media buffer is circle buffer */
//#define RB_PCMBUF_USE_MEDIA_BUFFER 1

/* Internals */
#define _LOG_TAG "[rbpcmbuf] "

#ifdef DEBUG
#define _LOG_DBG(str,...) TRACE(_LOG_TAG""str, ##__VA_ARGS__)
#define _LOG_ERR(str,...) TRACE(_LOG_TAG"err:"""str, ##__VA_ARGS__)

#define _LOG_FUNC_LINE() TRACE(_LOG_TAG"%s:%d\n", __FUNCTION__, __LINE__)
#define _LOG_FUNC_IN() TRACE(_LOG_TAG"%s ++++\n", __FUNCTION__)
#define _LOG_FUNC_OUT() TRACE(_LOG_TAG"%s ----\n", __FUNCTION__)
#else
#define _LOG_DBG(str,...)
#define _LOG_ERR(str,...) TRACE(_LOG_TAG"err:"""str, ##__VA_ARGS__)

#define _LOG_FUNC_LINE()
#define _LOG_FUNC_IN()
#define _LOG_FUNC_OUT()
#endif

/* Internals */
#define RB_PCMBUF_DMA_BUFFER_SIZE (1152*2*2*2)
uint8_t _rb_pcmbuf_dma_buffer[RB_PCMBUF_DMA_BUFFER_SIZE] __attribute__ ((aligned(4)));

RB_PCMBUF_AUD_STATE_T _rb_pcmbuf_audio_state = RB_PCMBUF_AUD_STATE_STOP;

#ifndef RB_PCMBUF_USE_MEDIA_BUFFER
uint32_t _rb_pcmbuf_irq_counter = 0;
uint32_t _rb_pcmbuf_current_buffer_len = RB_PCMBUF_DMA_BUFFER_SIZE/2;
uint8_t *_rb_pcmbuf_current_buffer_ptr = _rb_pcmbuf_dma_buffer;
#endif

#define _RB_PCMBUF_WAITER_SIGNAL (0xcc)
osThreadId _rb_pcmbuf_waiter_tid = 0;

void _rb_pcmbuf_waiter_wait(void)
{
    if (_rb_pcmbuf_waiter_tid == 0)
        _rb_pcmbuf_waiter_tid = osThreadGetId();
    osSignalWait(_RB_PCMBUF_WAITER_SIGNAL, osWaitForever);
}

void _rb_pcmbuf_waiter_release(void)
{
    osSignalSet(_rb_pcmbuf_waiter_tid, _RB_PCMBUF_WAITER_SIGNAL);
}


uint32_t _rb_pcmbuf_playback_callback(uint8_t *buf, uint32_t len)
{
#ifndef RB_PCMBUF_USE_MEDIA_BUFFER
    _rb_pcmbuf_current_buffer_ptr = buf;
    _rb_pcmbuf_current_buffer_len = len;
    if (_rb_pcmbuf_irq_counter > 0) {
        _LOG_ERR("pcmbuf underflow");
    }
    ++_rb_pcmbuf_irq_counter;
    _rb_pcmbuf_waiter_release();
#else
    /* TODO */
#endif
    return len;
}

/* APIs */
void rb_pcmbuf_init(void)
{
    struct AF_STREAM_CONFIG_T stream_cfg;

    stream_cfg.bits = AUD_BITS_16; 
    stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
    stream_cfg.sample_rate = AUD_SAMPRATE_44100;
#if FPGA==0     
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#else
    stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#endif
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;                                                                                                                                                      
    stream_cfg.vol = 0x3;
    stream_cfg.handler = _rb_pcmbuf_playback_callback;
    stream_cfg.data_ptr = _rb_pcmbuf_dma_buffer;
    stream_cfg.data_size = RB_PCMBUF_DMA_BUFFER_SIZE;

    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

    _rb_pcmbuf_audio_state = RB_PCMBUF_AUD_STATE_STOP;

#ifndef RB_PCMBUF_USE_MEDIA_BUFFER
    _rb_pcmbuf_irq_counter = 1;
#endif
}
void *rb_pcmbuf_request_buffer(int *size)
{
#ifndef RB_PCMBUF_USE_MEDIA_BUFFER
    if (_rb_pcmbuf_current_buffer_len <= 0)
        _rb_pcmbuf_waiter_wait();

    if (_rb_pcmbuf_current_buffer_len > 0) {
        /* FIXME : may need to diable irq, should be atomic here */
        *size = _rb_pcmbuf_current_buffer_len;
        _rb_pcmbuf_current_buffer_len = 0;

        return _rb_pcmbuf_current_buffer_ptr;
    }
#else
    /* TODO */
#endif
    return 0;
}
void rb_pcmbuf_write(unsigned int size)
{
#ifndef RB_PCMBUF_USE_MEDIA_BUFFER
    --_rb_pcmbuf_irq_counter;
    if (_rb_pcmbuf_audio_state == RB_PCMBUF_AUD_STATE_STOP) {
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        _rb_pcmbuf_audio_state = RB_PCMBUF_AUD_STATE_START;
    }
#else
    /* TODO */
#endif
}
