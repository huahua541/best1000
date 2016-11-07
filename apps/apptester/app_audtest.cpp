#include "cmsis_os.h"
#include "hal_trace.h"
#include "app_thread.h"

#include "hal_timer.h"
#include "app_audtest_pattern.h"

#include "hal_aud.h"
#include "analog.h"
#include "pmu.h"
#include "audioflinger.h"
#include "audiobuffer.h"
#include "stdbool.h"
#include <string.h>
#include "eq_export.h"
#include "usb_audio.h"
#include "anc_process.h"
#include "app_utils.h"

#ifdef USB_AUDIO_96K
#define USB_AUDIO_SAMPLE_RATE           96000
#else // 48K or 44.1K
#define USB_AUDIO_SAMPLE_RATE           48000
#endif

#ifdef USB_AUDIO_24BIT
#define USB_AUDIO_CODEC_SAMPLE_SIZE     4
#else
#define USB_AUDIO_CODEC_SAMPLE_SIZE     2
#endif

#define USB_AUDIO_PLAYBACK_FRAME_SIZE   ((USB_AUDIO_SAMPLE_RATE + (1000 - 1)) / 1000 * USB_AUDIO_CODEC_SAMPLE_SIZE * 2)
#define USB_AUDIO_PLAYBACK_BUFF_SIZE    (USB_AUDIO_PLAYBACK_FRAME_SIZE * 16)

#define USB_AUDIO_CAPTURE_FRAME_SIZE    ((48000 + (1000 - 1)) / 1000 * 2 * 1)
#define USB_AUDIO_CAPTURE_BUFF_SIZE     (USB_AUDIO_CAPTURE_FRAME_SIZE * 16)

#define APP_TEST_PLAYBACK_BUFF_SIZE     (128 * 20)
#define APP_TEST_CAPTURE_BUFF_SIZE      (128 * 20)

#if (USB_AUDIO_PLAYBACK_BUFF_SIZE > APP_TEST_PLAYBACK_BUFF_SIZE)
#define REAL_PLAYBACK_BUFF_SIZE         USB_AUDIO_PLAYBACK_BUFF_SIZE
#else
#define REAL_PLAYBACK_BUFF_SIZE         APP_TEST_PLAYBACK_BUFF_SIZE
#endif

#if (USB_AUDIO_CAPTURE_BUFF_SIZE > APP_TEST_CAPTURE_BUFF_SIZE)
#define REAL_CAPTURE_BUFF_SIZE          USB_AUDIO_CAPTURE_BUFF_SIZE
#else
#define REAL_CAPTURE_BUFF_SIZE          APP_TEST_CAPTURE_BUFF_SIZE
#endif

#define ALIGNED4                        ALIGNED(4)

static uint8_t ALIGNED4 app_test_playback_buff[REAL_PLAYBACK_BUFF_SIZE];
static uint8_t ALIGNED4 app_test_capture_buff[REAL_CAPTURE_BUFF_SIZE];

static uint8_t usb_audio_playback_started;
static uint8_t usb_audio_capture_started;

uint32_t pcm_1ksin_more_data(uint8_t *buf, uint32_t len)
{
    static uint32_t nextPbufIdx = 0;
    uint32_t remain_size = len;
    uint32_t curr_size = 0;
    static uint32_t pcm_preIrqTime = 0;;
    uint32_t stime = 0;


    stime = hal_sys_timer_get();
    TRACE( "pcm_1ksin_more_data irqDur:%d readbuff:0x%08x %d\n ", TICKS_TO_MS(stime - pcm_preIrqTime), buf, len);
    pcm_preIrqTime = stime;

    //  TRACE("[pcm_1ksin_more_data] len=%d nextBuf:%d\n", len, nextPbufIdx);
    if (remain_size > sizeof(sinwave))
    {
        do{
            if (nextPbufIdx)
            {
                curr_size = sizeof(sinwave)-nextPbufIdx;
                memcpy(buf,&sinwave[nextPbufIdx/2], curr_size);
                remain_size -= curr_size;
                nextPbufIdx = 0;
            }
            else if (remain_size>sizeof(sinwave))
            {
                memcpy(buf+curr_size,sinwave,sizeof(sinwave));
                curr_size += sizeof(sinwave);
                remain_size -= sizeof(sinwave);
            }
            else
            {
                memcpy(buf+curr_size,sinwave,remain_size);
                nextPbufIdx = remain_size;
                remain_size = 0;
            }
        }while(remain_size);
    }
    else
    {
        if ((sizeof(sinwave) - nextPbufIdx) >= len)
        {
            memcpy(buf, &sinwave[nextPbufIdx/2],len);
            nextPbufIdx += len;
        }
        else
        {
            curr_size = sizeof(sinwave)-nextPbufIdx;
            memcpy(buf, &sinwave[nextPbufIdx/2],curr_size);
            nextPbufIdx = len - curr_size;
            memcpy(buf+curr_size,sinwave, nextPbufIdx);
        }
    }


#ifdef EQ_PROCESS
      process_dsp(buf, NULL, len/4);
#endif
    return 0;
}

uint32_t pcm_mute_more_data(uint8_t *buf, uint32_t len)
{
    memset(buf, 0 , len);
    return 0;
}

void da_output_sin1k(bool  on)
{
    static bool isRun =  false;
    struct AF_STREAM_CONFIG_T stream_cfg;
    memset(&stream_cfg, 0, sizeof(stream_cfg));

    if (isRun==on)
        return;
    else
        isRun=on;
    TRACE("%s %d\n", __func__, on);

    if (on){
        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = AUD_SAMPRATE_44100;

        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.vol = 0x03;

        stream_cfg.handler = pcm_1ksin_more_data;
        stream_cfg.data_ptr = app_test_playback_buff;
        stream_cfg.data_size = APP_TEST_PLAYBACK_BUFF_SIZE;

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }else{
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }

}

void da_tester(uint8_t on)
{
#ifdef EQ_PROCESS
    init_dsp();
#endif
	da_output_sin1k(on);
}

extern int voicecvsd_audio_init(void);
extern uint32_t voicecvsd_audio_more_data(uint8_t *buf, uint32_t len);
extern int get_voicecvsd_buffer_size(void);
extern int store_voice_pcm2cvsd(unsigned char *buf, unsigned int len);

static uint32_t pcm_data_capture(uint8_t *buf, uint32_t len)
{
    uint32_t stime, etime;
    static uint32_t preIrqTime = 0;
    static uint32_t num = 0;
/*
    if(num==0)
    {
        dump_data2sd(1, NULL, 0);
//        TRACE("[%s] Open file!!!", __func__);
    }
    else if(num<200)
    {
        dump_data2sd(2, buf, len);
//        TRACE("[%s] write file!!!", __func__);
    }
    else if(num==200)
    {
        dump_data2sd(0, NULL, 0);
//        TRACE("[%s] Close file!!!", __func__);
    }
    else
    {
        TRACE("[%s] Close file!!!", __func__);
    }
    num++;
*/
    stime = hal_sys_timer_get();
//  audio_buffer_set_stereo2mono_16bits(buf, len, 1);
    audio_buffer_set(buf, len);
    etime = hal_sys_timer_get();
    TRACE("%s irqDur:%d fsSpend:%d, Len:%d", __func__, TICKS_TO_MS(stime - preIrqTime), TICKS_TO_MS(etime - stime), len);
    preIrqTime = stime;
    return 0;
}

uint32_t pcm_data_playback(uint8_t *buf, uint32_t len)
{
    uint32_t stime, etime;
    static uint32_t preIrqTime = 0;
    stime = hal_sys_timer_get();
//  audio_buffer_get_mono2stereo_16bits(buf, len);
    audio_buffer_get(buf, len);
    etime = hal_sys_timer_get();
    TRACE("%s irqDur:%d fsSpend:%d, Len:%d", __func__, TICKS_TO_MS(stime - preIrqTime), TICKS_TO_MS(etime - stime), len);
    preIrqTime = stime;
    return 0;
}

uint32_t pcm_cvsd_data_capture(uint8_t *buf, uint32_t len)
{
    int n;
    uint32_t stime, etime;
    static uint32_t preIrqTime = 0;

//    TRACE("%s enter", __func__);
    stime = hal_sys_timer_get();
    len >>= 1;
    audio_stereo2mono_16bits(0, (uint16_t *)buf, (uint16_t *)buf, len);

    store_voice_pcm2cvsd(buf, len);
    etime = hal_sys_timer_get();
    TRACE("[%s] exit irqDur:%d fsSpend:%d, add:%d remain:%d", __func__, TICKS_TO_MS(stime - preIrqTime), TICKS_TO_MS(etime - stime), len, n);
    preIrqTime = stime;
    return 0;
}

uint32_t pcm_cvsd_data_playback(uint8_t *buf, uint32_t len)
{
    int n;
    uint32_t stime, etime;
    static uint32_t preIrqTime = 0;

//    TRACE("%s enter", __func__);
    stime = hal_sys_timer_get();
//    pcm_1ksin_more_data(buf, len);
    voicecvsd_audio_more_data(buf, len);
    n = get_voicecvsd_buffer_size();
    etime = hal_sys_timer_get();
    TRACE("[%s] exit irqDur:%d fsSpend:%d, get:%d remain:%d", __func__, TICKS_TO_MS(stime - preIrqTime), TICKS_TO_MS(etime - stime), len, n);
    preIrqTime = stime;
    return 0;
}

static void usb_audio_playback_start(uint32_t start)
{
    int ret;

    if (start == usb_audio_playback_started) {
        return;
    }
    usb_audio_playback_started = start;

    if (start) {
        if (usb_audio_capture_started) {
            af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        }
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        if (usb_audio_capture_started) {
            af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        }
        ret = usb_audio_start_recv(app_test_playback_buff, USB_AUDIO_PLAYBACK_BUFF_SIZE / 2, USB_AUDIO_PLAYBACK_BUFF_SIZE);
        TRACE("usb_audio_start_recv: %d", ret);
    } else {
        usb_audio_stop_recv();
        if (usb_audio_capture_started) {
            af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        }
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        if (usb_audio_capture_started) {
            af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        }
    }
}

static void usb_audio_capture_start(uint32_t start)
{
    int ret;

    if (start == usb_audio_capture_started) {
        return;
    }
    usb_audio_capture_started = start;

    if (start) {
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        ret = usb_audio_start_send(app_test_capture_buff, USB_AUDIO_CAPTURE_BUFF_SIZE / 2, USB_AUDIO_CAPTURE_BUFF_SIZE);
        TRACE("usb_audio_start_send: %d", ret);
    } else {
        usb_audio_stop_send();
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    }
}

static void usb_audio_mute_control(uint32_t mute)
{
    if (mute) {
        analog_aud_coedc_mute();
    } else {
        analog_aud_coedc_nomute();
    }
}

static void usb_audio_vol_control(uint32_t percent)
{
    struct AF_STREAM_CONFIG_T *cfg;

    af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &cfg, true);
    cfg->vol = (percent + 5) / 10;
    af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, cfg);
}

static uint32_t usb_audio_data_playback(uint8_t *buf, uint32_t len)
{
#if 0
    uint32_t stime;
    uint32_t pos;
    static uint32_t preIrqTime = 0;

    pos = buf + len - app_test_playback_buff;
    if (pos >= USB_AUDIO_PLAYBACK_BUFF_SIZE) {
        pos = 0;
    }
    stime = hal_sys_timer_get();

    TRACE("%s irqDur:%d Len:%d pos:%d", __func__, TICKS_TO_MS(stime - preIrqTime), len, pos);

    preIrqTime = stime;
#endif
    return 0;
}

static uint32_t usb_audio_data_capture(uint8_t *buf, uint32_t len)
{
#if 0
    uint32_t stime;
    uint32_t pos;
    static uint32_t preIrqTime = 0;

    pos = buf + len - app_test_capture_buff;
    if (pos >= USB_AUDIO_CAPTURE_BUFF_SIZE) {
        pos = 0;
    }
    stime = hal_sys_timer_get();

    TRACE("%s irqDur:%d Len:%d pos:%d", __func__, TICKS_TO_MS(stime - preIrqTime), len, pos);

    preIrqTime = stime;
#endif
    return 0;
}

static void usb_audio_data_recv_handler(const uint8_t *data, uint32_t size, int error)
{
    if (error) {
        return;
    }
}

static void usb_audio_data_send_handler(const uint8_t *data, uint32_t size, int error)
{
    if (error) {
        return;
    }
}

void adc_looptester(bool on, enum AUD_IO_PATH_T input_path, enum AUD_SAMPRATE_T sample_rate)
{
    struct AF_STREAM_CONFIG_T stream_cfg;

    static bool isRun =  false;

    if (isRun==on)
        return;
    else
        isRun=on;

    if (on){

        audio_buffer_init();
        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = sample_rate;
        stream_cfg.vol = 0x03;

        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
//        stream_cfg.device = AUD_STREAM_USE_INT_SPDIF;

        stream_cfg.data_ptr = app_test_capture_buff;
        stream_cfg.data_size = APP_TEST_CAPTURE_BUFF_SIZE;

        stream_cfg.handler = pcm_data_capture;

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
//        stream_cfg.device = AUD_STREAM_USE_INT_SPDIF;

        stream_cfg.handler = pcm_data_playback;

        stream_cfg.data_ptr = app_test_playback_buff;
        stream_cfg.data_size = APP_TEST_PLAYBACK_BUFF_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);


        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    }else{
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }

}

void anc_tester(bool on, enum AUD_IO_PATH_T input_path, enum AUD_SAMPRATE_T sample_rate)
{
    struct AF_STREAM_CONFIG_T stream_cfg;

    static bool isRun =  false;

    TRACE("%s enter", __func__);

    if (isRun==on)
        return;
    else
        isRun=on;

    if (on){
        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = sample_rate;

        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.io_path = input_path;
        stream_cfg.vol = 0x01;

        stream_cfg.handler = NULL;

        stream_cfg.data_ptr = app_test_capture_buff;
        stream_cfg.data_size = APP_TEST_CAPTURE_BUFF_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        stream_cfg.handler = NULL;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

        stream_cfg.data_ptr = app_test_playback_buff;
        stream_cfg.data_size = APP_TEST_PLAYBACK_BUFF_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        anc_enable();
    }else{
        anc_disable();
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }
}

void usb_audio_tester(bool on)
{
    static const struct USB_AUDIO_CFG_T usb_audio_cfg = {
        .recv_start_callback = usb_audio_playback_start,
        .send_start_callback = usb_audio_capture_start,
        .mute_callback = usb_audio_mute_control,
        .vol_callback = usb_audio_vol_control,
        .recv_callback = usb_audio_data_recv_handler,
        .send_callback = usb_audio_data_send_handler,
    };
    struct AF_STREAM_CONFIG_T stream_cfg;
    enum AUD_SAMPRATE_T sample_rate_play;
    enum AUD_SAMPRATE_T sample_rate_capture;

    static bool isRun =  false;

    if (isRun==on)
        return;
    else
        isRun=on;

    TRACE("%s: on=%d", __FUNCTION__, on);

    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);

#ifdef USB_AUDIO_96K
    sample_rate_play = AUD_SAMPRATE_96000;
    sample_rate_capture = AUD_SAMPRATE_48000;
#elif defined(USB_AUDIO_44_1K)
    sample_rate_play = AUD_SAMPRATE_44100;
    sample_rate_capture = AUD_SAMPRATE_44100;
#else
    sample_rate_play = AUD_SAMPRATE_48000;
    sample_rate_capture = AUD_SAMPRATE_48000;
#endif

    if (on){
        usb_audio_playback_started = 0;
        usb_audio_capture_started = 0;

        memset(&stream_cfg, 0, sizeof(stream_cfg));

#ifdef USB_AUDIO_24BIT
        stream_cfg.bits = AUD_BITS_24;
#else
        stream_cfg.bits = AUD_BITS_16;
#endif
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = sample_rate_play;

        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.vol = 0x01;

        stream_cfg.handler = usb_audio_data_playback;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

        stream_cfg.data_ptr = app_test_playback_buff;
        stream_cfg.data_size = USB_AUDIO_PLAYBACK_BUFF_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
        stream_cfg.sample_rate = sample_rate_capture;
        stream_cfg.vol = 0x01;

        stream_cfg.handler = usb_audio_data_capture;
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;

        stream_cfg.data_ptr = app_test_capture_buff;
        stream_cfg.data_size = USB_AUDIO_CAPTURE_BUFF_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        pmu_usb_config(1);
        analog_usbdevice_init();

        usb_audio_open(&usb_audio_cfg);
    }else{
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        pmu_usb_config(0);
    }
}
