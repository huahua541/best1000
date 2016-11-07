#include "cmsis_os.h"
#include "plat_types.h"
#include "codec_best1000.h"
#include "reg_codecip.h"
#include "hal_codecip.h"
#include "hal_codec.h"
#include "hal_trace.h"
#include "hal_sysfreq.h"
#include "stdbool.h"
#include "analog.h"

#define CODEC_TRACE(s,...)
//TRACE(s, ##__VA_ARGS__)

#define CODEC_BEST1000_INST HAL_CODEC_ID_0

#ifdef __CODEC_ASYNC_CLOSE__
#define CODEC_ASYNC_CLOSE_DELAY (5000)
#endif

#ifdef __CODEC_ASYNC_CLOSE__
static void codec_timehandler(void const *param);
osTimerDef (CODEC_TIMER, codec_timehandler);
#endif

// DAC reset will affect ADC, so DAC initialization must be done before ADC.
// Next digital metal change will fix this issue, and this workaround 
// can be skipped for future chips.
#define CODEC_PLAY_BEFORE_CAPTURE

#ifdef CODEC_PLAY_BEFORE_CAPTURE
static bool codec_capture_started;
#endif

struct BEST1000_CODEC_CONFIG_T{
    bool running;
    osTimerId timer;
    struct BEST1000_STEAM_CONFIG_T {
        bool running;
        struct HAL_CODEC_CONFIG_T codec_cfg;
    }stream_cfg[AUD_STREAM_NUM];
};
static struct BEST1000_CODEC_CONFIG_T codec_best1000_cfg = {
    .running =  false,
    .timer = NULL,
    //playback
    .stream_cfg[AUD_STREAM_PLAYBACK] = {
                        .running =  false,
                        .codec_cfg = {
                            .bits = AUD_BITS_16,
                            .sample_rate = AUD_SAMPRATE_44100,
                            .channel_num = AUD_CHANNEL_NUM_2,
                            .use_dma = true,
                            .io_path = AUD_OUTPUT_PATH_SPEAKER,
                            .vol = 0x03,
                            .set_flag = HAL_CODEC_CONFIG_ALL,
                        }
    },
    //capture
    .stream_cfg[AUD_STREAM_CAPTURE] = {
                        .running =  false,
                        .codec_cfg = {
                            .bits = AUD_BITS_16,
                            .sample_rate = AUD_SAMPRATE_44100,
                            .channel_num = AUD_CHANNEL_NUM_2,
                            .use_dma = true,
                            .io_path = AUD_INPUT_PATH_MAINMIC,
                            .vol = 0x03,
                            .set_flag = HAL_CODEC_CONFIG_ALL,
                        }
    }
};


uint32_t codec_best1000_stream_open(enum AUD_STREAM_T stream)
{
    CODEC_TRACE("%s:%d\n", __func__, stream);
    codec_best1000_cfg.stream_cfg[stream].running = true;
    codec_best1000_cfg.stream_cfg[stream].codec_cfg.set_flag = HAL_CODEC_CONFIG_ALL;
    hal_codec_setup_stream(CODEC_BEST1000_INST, stream, &(codec_best1000_cfg.stream_cfg[stream].codec_cfg));

    return 0;
}


//依据输入参数(stream 和 cfg) 来配置相关寄存器，包括: PLL, Digital, Analog, ADC, DAC等。
uint32_t codec_best1000_stream_setup(enum AUD_STREAM_T stream, struct HAL_CODEC_CONFIG_T *cfg)
{
    codec_best1000_cfg.stream_cfg[stream].codec_cfg.set_flag = HAL_CODEC_CONFIG_NULL;

    if(codec_best1000_cfg.stream_cfg[stream].codec_cfg.bits != cfg->bits)
    {
        CODEC_TRACE("[bits]old = %d, new = %d", codec_best1000_cfg.stream_cfg[stream].codec_cfg.bits, cfg->bits);
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.bits = cfg->bits;
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_BITS;
    }

    if(codec_best1000_cfg.stream_cfg[stream].codec_cfg.sample_rate != cfg->sample_rate)
    {
        CODEC_TRACE("[sample_rate]old = %d, new = %d", codec_best1000_cfg.stream_cfg[stream].codec_cfg.sample_rate, cfg->sample_rate);
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.sample_rate = cfg->sample_rate;
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_SMAPLE_RATE;
    }

    if(codec_best1000_cfg.stream_cfg[stream].codec_cfg.channel_num != cfg->channel_num)
    {
        CODEC_TRACE("[channel_num]old = %d, new = %d", codec_best1000_cfg.stream_cfg[stream].codec_cfg.channel_num, cfg->channel_num);
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.channel_num = cfg->channel_num;
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_CHANNEL_NUM;
    }

    if(codec_best1000_cfg.stream_cfg[stream].codec_cfg.use_dma != cfg->use_dma)
    {
        CODEC_TRACE("[use_dma]old = %d, new = %d", codec_best1000_cfg.stream_cfg[stream].codec_cfg.use_dma, cfg->use_dma);
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.use_dma = cfg->use_dma;
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_USE_DMA;
    }

    if(codec_best1000_cfg.stream_cfg[stream].codec_cfg.vol != cfg->vol)
    {
        CODEC_TRACE("[vol]old = %d, new = %d", codec_best1000_cfg.stream_cfg[stream].codec_cfg.vol, cfg->vol);
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.vol = cfg->vol;
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_VOL;
    }

    if(codec_best1000_cfg.stream_cfg[stream].codec_cfg.io_path != cfg->io_path)
    {
        CODEC_TRACE("[io_path]old = %d, new = %d", codec_best1000_cfg.stream_cfg[stream].codec_cfg.io_path, cfg->io_path);
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.io_path = cfg->io_path;
        codec_best1000_cfg.stream_cfg[stream].codec_cfg.set_flag |= HAL_CODEC_CONFIG_IO_PATH;
    }

    CODEC_TRACE("[%s]stream = %d, set_flag = %x",__func__,stream,codec_best1000_cfg.stream_cfg[stream].codec_cfg.set_flag);
    hal_codec_setup_stream(CODEC_BEST1000_INST, stream, &(codec_best1000_cfg.stream_cfg[stream].codec_cfg));

    return 0;
}

uint32_t codec_best1000_stream_start(enum AUD_STREAM_T stream)
{
    if (stream == AUD_STREAM_PLAYBACK){
#ifdef CODEC_PLAY_BEFORE_CAPTURE
        ASSERT(!codec_capture_started, "CODEC play should be started first");
#endif
        analog_aud_coedc_mute();
        hal_cmu_reset_pulse(HAL_CMU_MOD_O_CODEC_DA);
        hal_codec_start_stream(CODEC_BEST1000_INST, stream);
        //analog_aud_codec_speaker_enable(true);
        analog_aud_coedc_nomute();
    }else{
#ifdef CODEC_PLAY_BEFORE_CAPTURE
        codec_capture_started = true;
#endif
        hal_cmu_reset_pulse(HAL_CMU_MOD_O_CODEC_AD);
        hal_codec_start_stream(CODEC_BEST1000_INST, stream);
        analog_aud_codec_mic_enable(true);
        analog_aud_coedc_nomute();
    }

    return 0;
}
uint32_t codec_best1000_stream_stop(enum AUD_STREAM_T stream)
{
    if (stream == AUD_STREAM_PLAYBACK){
#ifdef CODEC_PLAY_BEFORE_CAPTURE
        ASSERT(!codec_capture_started, "CODEC play should be stopped last");
#endif
        //analog_aud_codec_speaker_enable(false);
        analog_aud_coedc_mute();
        hal_codec_stop_stream(CODEC_BEST1000_INST, stream);
        hal_cmu_reset_pulse(HAL_CMU_MOD_O_CODEC_DA);
    }else{
        analog_aud_codec_mic_enable(false);
        hal_codec_stop_stream(CODEC_BEST1000_INST, stream);
        hal_cmu_reset_pulse(HAL_CMU_MOD_O_CODEC_AD);
#ifdef CODEC_PLAY_BEFORE_CAPTURE
        codec_capture_started = false;
#endif
    }

    return 0;
}
uint32_t codec_best1000_stream_close(enum AUD_STREAM_T stream)
{
    //close all stream
    CODEC_TRACE("%s:%d\n", __func__, stream);
    codec_best1000_cfg.stream_cfg[stream].running = false;
    return 0;
}

#ifdef __CODEC_ASYNC_CLOSE__
uint32_t codec_best1000_open(void)
{
    CODEC_TRACE("%s:%d\n", __func__, codec_best1000_cfg.running);
    if (codec_best1000_cfg.timer == NULL)
        codec_best1000_cfg.timer = osTimerCreate (osTimer(CODEC_TIMER), osTimerOnce, NULL);

    osTimerStop(codec_best1000_cfg.timer);
#ifdef __AUDIO_RESAMPLE__
    // Different clock source for audio resample -- Must be reconfigured here
    analog_aud_pll_open();
#endif
    if (!codec_best1000_cfg.running){
        CODEC_TRACE("trig codec open");
        hal_sysfreq_req(HAL_SYSFREQ_USER_CODEC, HAL_CMU_FREQ_26M);
        hal_codec_sdm_gain_set(CODEC_BEST1000_INST, 0);     //??? This is used to mute? should rename
#ifndef __AUDIO_RESAMPLE__
        analog_aud_pll_open();
#endif
        analog_aud_coedc_open(AUD_INPUT_PATH_MAINMIC);
        hal_codec_open(CODEC_BEST1000_INST);
        codec_best1000_cfg.running = true;
    }

    return 0;
}

uint32_t codec_best1000_close(void)
{
    CODEC_TRACE("%s:%d\n", __func__, codec_best1000_cfg.running);

#ifdef __AUDIO_RESAMPLE__
    // Different clock source for audio resample -- Close now and reconfigure when open
    analog_aud_pll_close();
#endif
    if (codec_best1000_cfg.running &&
        (codec_best1000_cfg.stream_cfg[AUD_STREAM_PLAYBACK].running == false) &&
        (codec_best1000_cfg.stream_cfg[AUD_STREAM_CAPTURE].running == false)){
        CODEC_TRACE("trig codec close(try)");
        osTimerStop(codec_best1000_cfg.timer);
        osTimerStart(codec_best1000_cfg.timer, CODEC_ASYNC_CLOSE_DELAY);
    }
    return 0;
}

uint32_t codec_best1000_real_close(void)
{
    if (codec_best1000_cfg.running){
        hal_codec_close(CODEC_BEST1000_INST);
        analog_aud_coedc_close();
#ifndef __AUDIO_RESAMPLE__
        analog_aud_pll_close();
#endif
        hal_sysfreq_req(HAL_SYSFREQ_USER_CODEC, HAL_CMU_FREQ_32K);
        codec_best1000_cfg.running = false;
    }
    return 0;
}

static void codec_timehandler(void const *param)
{
    codec_best1000_real_close();
}
#else
uint32_t codec_best1000_open(void)
{
    CODEC_TRACE("%s:%d\n", __func__, codec_best1000_cfg.running);

    if (!codec_best1000_cfg.running){
        CODEC_TRACE("trig codec open");
        hal_sysfreq_req(HAL_SYSFREQ_USER_CODEC, HAL_CMU_FREQ_26M);
        hal_codec_sdm_gain_set(CODEC_BEST1000_INST, 0);     //??? This is used to mute? should rename
        analog_aud_pll_open();
        analog_aud_coedc_open(AUD_INPUT_PATH_MAINMIC);
        hal_codec_open(CODEC_BEST1000_INST);
        codec_best1000_cfg.running = true;
    }

    return 0;
}

uint32_t codec_best1000_close(void)
{
    CODEC_TRACE("%s:%d\n", __func__, codec_best1000_cfg.running);

    if (codec_best1000_cfg.running &&
        (codec_best1000_cfg.stream_cfg[AUD_STREAM_PLAYBACK].running == false) &&
        (codec_best1000_cfg.stream_cfg[AUD_STREAM_CAPTURE].running == false)){
        CODEC_TRACE("trig codec close");
        hal_codec_close(CODEC_BEST1000_INST);
        analog_aud_coedc_close();
        analog_aud_pll_close();
        hal_sysfreq_req(HAL_SYSFREQ_USER_CODEC, HAL_CMU_FREQ_32K);
        codec_best1000_cfg.running = false;
    }
    return 0;
}
#endif
