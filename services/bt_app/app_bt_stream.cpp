//#include "mbed.h"
#include <stdio.h>
#include <assert.h>

#include "cmsis_os.h"
#include "tgt_hardware.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_chipid.h"
#include "analog.h"
#include "app_bt_stream.h"
#include "app_overlay.h"
#include "app_audio.h"
#include "app_utils.h"

#ifdef MEDIA_PLAYER_SUPPORT
#include "resources.h"
#include "app_media_player.h"
#endif
#ifdef __FACTORY_MODE_SUPPORT__
#include "app_factory_audio.h"
#endif

#include "app_ring_merge.h"

#include "bt_drv.h"
#include "bt_xtal_sync.h"
#include "iabt.h"
extern "C" {
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "a2dp.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "hf.h"
}


#include "cqueue.h"
#include "btapp.h"

#ifdef EQ_PROCESS
#include "eq_export.h"
#endif

#if defined(__HW_FIR_EQ_PROCESS__)||defined(__SW_IIR_EQ_PROCESS__)
#include "audio_eq.h"
#endif

#ifdef __HW_FIR_EQ_PROCESS__
#include "res_audio_eq.h"
#endif

#include "app_bt_media_manager.h"

#include "string.h"
#include "hal_location.h"

enum PLAYER_OPER_T {
    PLAYER_OPER_START,
    PLAYER_OPER_STOP,
    PLAYER_OPER_RESTART,
};

#if (AUDIO_OUTPUT_VOLUME_DEFAULT < 1) || (AUDIO_OUTPUT_VOLUME_DEFAULT > 16)
#error "AUDIO_OUTPUT_VOLUME_DEFAULT out of range"
#endif
int8_t bt_stream_volume = (AUDIO_OUTPUT_VOLUME_DEFAULT);

uint32_t a2dp_audio_more_data(uint8_t *buf, uint32_t len);
int a2dp_audio_init(void);
int a2dp_audio_deinit(void);

enum AUD_SAMPRATE_T a2dp_sample_rate = AUD_SAMPRATE_48000;


int bt_sbc_player_setup(uint8_t freq)
{
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    static uint8_t sbc_samp_rate = 0xff;

    if (sbc_samp_rate == freq)
        return 0;

    switch (freq) {
        case A2D_SBC_IE_SAMP_FREQ_16:
        case A2D_SBC_IE_SAMP_FREQ_32:
        case A2D_SBC_IE_SAMP_FREQ_48:
            a2dp_sample_rate = AUD_SAMPRATE_48000;
            break;
        case A2D_SBC_IE_SAMP_FREQ_44:
            a2dp_sample_rate = AUD_SAMPRATE_44100;
            break;
        default:
            break;
    }

    af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
    stream_cfg->sample_rate = a2dp_sample_rate;
    af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
    sbc_samp_rate = freq;

    return 0;
}


//static uint32_t bt_sbc_player_audio_more_data(uint8_t *buf, uint32_t len)
//{
//    uint32_t stime,etime1,etime2;
//
//    stime = hal_sys_timer_get();
//    a2dp_audio_more_data(buf, len);
//    etime1 = hal_sys_timer_get();

//#ifdef EQ_PROCESS
//    process_dsp(buf, NULL, len/4);
//#endif

//    etime2 = hal_sys_timer_get();
//    TRACE( "sbc dec:%d dsp:%d",TICKS_TO_MS(etime1 - stime), TICKS_TO_MS(etime2 - etime1));
//}
void merge_stereo_to_mono_16bits(int16_t *src_buf, int16_t *dst_buf,  uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; i+=2) {
        dst_buf[i] = (src_buf[i]>>1) + (src_buf[i+1]>>1);
        dst_buf[i+1] = dst_buf[i];
    }
}

#ifdef __AUDIO_RESAMPLE__
#include "audio_resample.h"
#define SAMPLE_NUM 128
short raw_pcm_data[SAMPLE_NUM * 2];

enum resample_id_t bt_get_up_sample_id(enum AUD_SAMPRATE_T sample_rate)
{
    enum resample_id_t resample_id = RESAMPLE_ID_NUM;

    if(a2dp_sample_rate == AUD_SAMPRATE_44100)
    {
        resample_id = RESAMPLE_ID_44P1TO50P7;
    }
    else if(a2dp_sample_rate == AUD_SAMPRATE_48000)
    {
        resample_id = RESAMPLE_ID_48TO50P7;
    }
    else
    {
        TRACE("[%s] Do not support %d sample rate to resample", __func__, sample_rate);
    }

    return resample_id;
}
#endif

uint32_t bt_sbc_player_more_data(uint8_t *buf, uint32_t len)
{
#ifdef BT_XTAL_SYNC
    bt_xtal_sync();
#endif

#ifdef __AUDIO_RESAMPLE__
    int res;
//    int32_t stime,etime;


    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_cmu_get_audio_resample_status())
    {
        res = audio_resample_cfg((short *)buf, len/2);
        while(1)
        {
            //decode raw pcm data

            a2dp_audio_more_data((uint8_t *)raw_pcm_data, SAMPLE_NUM*2*2);

//            stime = hal_sys_timer_get();
//
//            uint32_t lock;
//            lock = int_lock();
            res = audio_resample_run(raw_pcm_data, SAMPLE_NUM*2);
//            int_unlock(lock);
//
//            etime = hal_sys_timer_get();
//
//            TRACE( "resample: %d ", etime - stime);

            if(res == AUDIO_RESAMPLE_STATUS_SUCESS)
            {
                break;
            }

        }
    }
    else
#endif
    {
        a2dp_audio_more_data(buf, len);
    }
    app_ring_merge_more_data(buf, len);
#ifdef __AUDIO_OUTPUT_MONO_MODE__
    merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)buf, len/2);
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    audio_eq_fir_run(buf, len);
#endif

#ifdef __SW_IIR_EQ_PROCESS__
	audio_eq_iir_run(buf, len);
#endif
    return len;
}

#ifdef __AUDIO_RESAMPLE__
void* audio_resample_get_ext_buff(int size)
{
    uint8_t *pBuff = NULL;
    size = size +(4 - size % 4);
    app_audio_mempool_get_buff(&pBuff, size);
    TRACE( "audio_resample_get_ext_buff len:%d", size);
    return (void*)pBuff;
}
#endif

int bt_sbc_player(enum PLAYER_OPER_T on, enum APP_SYSFREQ_FREQ_T freq)
{
    struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;
    uint8_t* bt_audio_buff = NULL;

    TRACE("bt_sbc_player work:%d op:%d freq:%d", isRun, on, freq);

    if ((isRun && on == PLAYER_OPER_START) || (!isRun && on == PLAYER_OPER_STOP))
        return 0;

    if (on == PLAYER_OPER_STOP || on == PLAYER_OPER_RESTART) {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#ifdef __AUDIO_RESAMPLE__
        if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_cmu_get_audio_resample_status())
        {
            audio_resample_close();
        }
#endif
#ifdef __HW_FIR_EQ_PROCESS__
        audio_eq_fir_close();
#endif
#ifdef __SW_IIR_EQ_PROCESS__
		audio_eq_iir_close();
#endif

        if (on == PLAYER_OPER_STOP) {
#ifndef FPGA
#ifdef BT_XTAL_SYNC
            osDelay(50);
            bt_term_xtal_sync();
#endif
#endif
            a2dp_audio_deinit();
            app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
			af_set_priority(osPriorityAboveNormal);
        }
    }

    if (on == PLAYER_OPER_START || on == PLAYER_OPER_RESTART) {
		af_set_priority(osPriorityHigh);
        if (freq < APP_SYSFREQ_52M) {
            freq = APP_SYSFREQ_52M;
        }
        app_audio_mempool_init();
        app_audio_mempool_get_buff(&bt_audio_buff, BT_AUDIO_BUFF_SIZE);
#if defined(PC_CMD_ENABLE)
		app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_208M);
#elif defined(__HW_FIR_EQ_PROCESS__)&&defined(__SW_IIR_EQ_PROCESS__)
		if (RES_AUDIO_EQ_COEF.len > 128){
			app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq>APP_SYSFREQ_104M?freq:APP_SYSFREQ_104M);
		}else{
			app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);
		}
#else
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);
#endif

        memset(&stream_cfg, 0, sizeof(stream_cfg));
        memset(bt_audio_buff, 0, BT_AUDIO_BUFF_SIZE);
        if (on == PLAYER_OPER_START) {
#ifndef FPGA
            app_overlay_select(APP_OVERLAY_A2DP);
#ifdef BT_XTAL_SYNC
            bt_init_xtal_sync();
#endif
#endif
        }
#ifdef EQ_PROCESS
        init_dsp();
#endif
#ifdef __AUDIO_RESAMPLE__
        if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_cmu_sys_get_freq() <= HAL_CMU_FREQ_52M)
        {
            enum resample_id_t resample_id;
            resample_id = bt_get_up_sample_id(a2dp_sample_rate);
            if (resample_id != RESAMPLE_ID_NUM)
            {
                audio_resample_open(resample_id, audio_resample_get_ext_buff);
            }
        }
#endif

        if (on == PLAYER_OPER_START) {
            a2dp_audio_init();
        }
		
#ifdef __SW_IIR_EQ_PROCESS__
		uint8_t i;
		audio_eq_iir_open(sizeof(cfg_aud_eq_iir_band_settings)/sizeof(TGT_AUD_EQ_IIR_BAND_SETTINGS),a2dp_sample_rate);
		for (i = 0; i < sizeof(cfg_aud_eq_iir_band_settings)/sizeof(TGT_AUD_EQ_IIR_BAND_SETTINGS); i++) {
			audio_eq_iir_set_cfg(i, (AUDIO_IIR_CFG *)&cfg_aud_eq_iir_band_settings[i]);
		}	
		audio_eq_iir_update_coefs();
#endif

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = a2dp_sample_rate;
#ifdef FPGA
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#endif
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.vol = bt_stream_volume;
        stream_cfg.handler = bt_sbc_player_more_data;

        stream_cfg.data_ptr = bt_audio_buff;
        stream_cfg.data_size = BT_AUDIO_BUFF_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#ifdef __HW_FIR_EQ_PROCESS__
        audio_eq_fir_open(&RES_AUDIO_EQ_COEF, (int)stream_cfg.sample_rate, NULL);
#endif
    }

    isRun = (on != PLAYER_OPER_STOP);
    return 0;
}


int voicebtpcm_pcm_audio_init(void);
uint32_t voicebtpcm_pcm_audio_data_come(uint8_t *buf, uint32_t len);
uint32_t voicebtpcm_pcm_audio_more_data(uint8_t *buf, uint32_t len);
int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len);
int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len);

//( codec:mic-->btpcm:tx
// codec:mic
static uint32_t bt_sco_codec_capture_data(uint8_t *buf, uint32_t len)
{
    voicebtpcm_pcm_audio_data_come(buf, len);
    return len;
}

// btpcm:tx
static uint32_t bt_sco_btpcm_playback_data(uint8_t *buf, uint32_t len)
{
    get_voicebtpcm_p2m_frame(buf, len);
    return len;
}
//)

//( btpcm:rx-->codec:spk
// btpcm:rx
static uint32_t bt_sco_btpcm_capture_data(uint8_t *buf, uint32_t len)
{
    store_voicebtpcm_m2p_buffer(buf, len);

    return len;
}

// codec:spk
static uint32_t bt_sco_codec_playback_data(uint8_t *buf, uint32_t len)
{
#ifdef BT_XTAL_SYNC
    bt_xtal_sync();
#endif

    voicebtpcm_pcm_audio_more_data(buf, len);
    app_ring_merge_more_data(buf, len);

    return len;
}
//)

int bt_sco_player(bool on, enum APP_SYSFREQ_FREQ_T freq)
{
    struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;
    uint8_t * bt_audio_buff = NULL;
    
    TRACE("bt_sco_player work:%d op:%d freq:%d", isRun, on, freq);
//  osDelay(1);

    if (isRun==on)
        return 0;

    if (on){

         app_audio_mempool_init();
         app_audio_mempool_get_buff(&bt_audio_buff, BT_AUDIO_SCO_BUFF_SIZE*4);
        //bt_syncerr set to max(0x0a)
//        BTDIGITAL_REG_SET_FIELD(REG_BTCORE_BASE_ADDR, 0x0f, 0, 0x0f);
//        af_set_priority(osPriorityRealtime);

#if defined(SPEECH_NOISE_SUPPRESS) && defined(SPEECH_ECHO_CANCEL) && !defined(_SCO_BTPCM_CHANNEL_)
        if (freq < APP_SYSFREQ_104M) {
            freq = APP_SYSFREQ_104M;
        }
#else
        if (freq < APP_SYSFREQ_52M) {
            freq = APP_SYSFREQ_52M;
        }
#endif
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);

#ifndef FPGA
        app_overlay_select(APP_OVERLAY_HFP);
#ifdef BT_XTAL_SYNC
        bt_init_xtal_sync();
#endif
#endif

        voicebtpcm_pcm_audio_init();
        memset(&stream_cfg, 0, sizeof(stream_cfg));
        memset(bt_audio_buff, 0, BT_AUDIO_SCO_BUFF_SIZE*4);
        memset(&hf_sendbuff_ctrl, 0, sizeof(hf_sendbuff_ctrl));

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
        stream_cfg.sample_rate = AUD_SAMPRATE_8000;
        stream_cfg.vol = bt_stream_volume;
        stream_cfg.data_size = BT_AUDIO_SCO_BUFF_SIZE;

        // codec:mic
#ifdef FPGA
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#endif
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
        stream_cfg.handler = bt_sco_codec_capture_data;
        stream_cfg.data_ptr = bt_audio_buff;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        // codec:spk
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = bt_sco_codec_playback_data;
        stream_cfg.data_ptr = bt_audio_buff + BT_AUDIO_SCO_BUFF_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

#ifdef _SCO_BTPCM_CHANNEL_
        // btpcm:rx
        stream_cfg.device = AUD_STREAM_USE_BT_PCM;
        stream_cfg.handler = bt_sco_btpcm_capture_data;
        stream_cfg.data_ptr = bt_audio_buff + BT_AUDIO_SCO_BUFF_SIZE * 2;
        af_stream_open(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE, &stream_cfg);

        // btpcm:tx
        stream_cfg.device = AUD_STREAM_USE_BT_PCM;
        stream_cfg.handler = bt_sco_btpcm_playback_data;
        stream_cfg.data_ptr = bt_audio_buff + BT_AUDIO_SCO_BUFF_SIZE * 3;
        af_stream_open(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK, &stream_cfg);

        af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
#endif
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        TRACE("bt_sco_player on");
    } else {
#ifndef FPGA
		osDelay(100);        //del by wanghaihui for discon two call the af_dma_irq_handler disappear on FPGA
#endif
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#ifdef _SCO_BTPCM_CHANNEL_
        af_stream_stop(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);

        af_stream_close(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
#endif
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#ifndef FPGA
#ifdef BT_XTAL_SYNC
        osDelay(50);
        bt_term_xtal_sync();
#endif
#endif
        TRACE("bt_sco_player off");
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
 //       af_set_priority(osPriorityAboveNormal);

        //bt_syncerr set to default(0x07)
 //       BTDIGITAL_REG_SET_FIELD(REG_BTCORE_BASE_ADDR, 0x0f, 0, 0x07);
    }

    isRun=on;
    return 0;
}

APP_BT_STREAM_T gStreamplayer = APP_BT_STREAM_NUM;
int app_bt_stream_open(APP_AUDIO_STATUS* status)
{
    int nRet = -1;
    APP_BT_STREAM_T player = (APP_BT_STREAM_T)status->id;
    enum APP_SYSFREQ_FREQ_T freq = (enum APP_SYSFREQ_FREQ_T)status->freq;

    TRACE("app_bt_stream_open prev:%d cur:%d freq:%d", gStreamplayer, player, freq);

    if (gStreamplayer != APP_BT_STREAM_NUM){
        TRACE("Close prev bt stream before opening");
        nRet = app_bt_stream_close(gStreamplayer);
        if (nRet)
            return -1;
    }

    switch (player) {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            nRet = bt_sco_player(true, freq);
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            nRet = bt_sbc_player(PLAYER_OPER_START, freq);
            break;
#ifdef __FACTORY_MODE_SUPPORT__
        case APP_FACTORYMODE_AUDIO_LOOP:
            nRet = app_factorymode_audioloop(true, freq);
            break;
#endif
   #ifdef MEDIA_PLAYER_SUPPORT
        case APP_PLAY_BACK_AUDIO:
            nRet = app_play_audio_onoff(true, status);
            break;
    #endif
        default:
            nRet = -1;
            break;
    }

    if (!nRet)
        gStreamplayer = player;

    return nRet;
}

int app_bt_stream_close(APP_BT_STREAM_T player)
{
    int nRet = -1;
    TRACE("app_bt_stream_close prev:%d cur:%d", gStreamplayer, player);
//  osDelay(1);

    if (gStreamplayer != player)
        return -1;

    switch (player) {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            nRet = bt_sco_player(false, APP_SYSFREQ_32K);
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            nRet = bt_sbc_player(PLAYER_OPER_STOP, APP_SYSFREQ_32K);
            break;
#ifdef __FACTORY_MODE_SUPPORT__
        case APP_FACTORYMODE_AUDIO_LOOP:
            nRet = app_factorymode_audioloop(false, APP_SYSFREQ_32K);
            break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
       case APP_PLAY_BACK_AUDIO:
            nRet = app_play_audio_onoff(false, NULL);
            break;
#endif
        default:
            nRet = -1;
            break;
    }
    if (!nRet)
        gStreamplayer = APP_BT_STREAM_NUM;
    return nRet;
}

int app_bt_stream_setup(APP_BT_STREAM_T player, uint8_t status)
{
    int nRet = -1;

    TRACE("app_bt_stream_setup prev:%d cur:%d sample:%d", gStreamplayer, player, status);

    switch (player) {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            bt_sbc_player_setup(status);
            break;
        default:
            nRet = -1;
            break;
    }

    return nRet;
}

int app_bt_stream_restart(APP_AUDIO_STATUS* status)
{
    int nRet = -1;
    APP_BT_STREAM_T player = (APP_BT_STREAM_T)status->id;
    enum APP_SYSFREQ_FREQ_T freq = (enum APP_SYSFREQ_FREQ_T)status->freq;

    TRACE("app_bt_stream_restart prev:%d cur:%d freq:%d", gStreamplayer, player, freq);

    if (gStreamplayer != player)
        return -1;

    switch (player) {
        case APP_BT_STREAM_HFP_PCM:
        case APP_BT_STREAM_HFP_CVSD:
        case APP_BT_STREAM_HFP_VENDOR:
            break;
        case APP_BT_STREAM_A2DP_SBC:
        case APP_BT_STREAM_A2DP_AAC:
        case APP_BT_STREAM_A2DP_VENDOR:
            nRet = bt_sbc_player(PLAYER_OPER_RESTART, freq);
            break;
#ifdef __FACTORY_MODE_SUPPORT__
        case APP_FACTORYMODE_AUDIO_LOOP:
            break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
        case APP_PLAY_BACK_AUDIO:
            break;
#endif
        default:
            nRet = -1;
            break;
    }

    return nRet;
}


int app_bt_stream_volumeup(void)
{
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;

    if (++bt_stream_volume > TGT_VOLUME_LEVE_15){
        bt_stream_volume = TGT_VOLUME_LEVE_15;
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_BT_WARNING,0);
#endif
    }else{
    	if (!app_bt_stream_isrun(APP_PLAY_BACK_AUDIO)){
	        af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
	        stream_cfg->vol = bt_stream_volume;
	        af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
    	}
    }
    return 0;
}

int app_bt_stream_volumedown(void)
{
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;

    if (--bt_stream_volume < TGT_VOLUME_LEVE_0){
        bt_stream_volume = TGT_VOLUME_LEVE_0;
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_BT_WARNING,0);
#endif
    }else{    
		if (!app_bt_stream_isrun(APP_PLAY_BACK_AUDIO)){
	        af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
	        stream_cfg->vol = bt_stream_volume;
	        af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
		}
    }
    return 0;
}

int app_bt_stream_volumeset(int8_t vol)
{
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
    TRACE("app_bt_stream_volumeset vol=%d", vol);

    if (vol > TGT_VOLUME_LEVE_15)
        vol = TGT_VOLUME_LEVE_15;
    if (vol < TGT_VOLUME_LEVE_0)
        vol = TGT_VOLUME_LEVE_0;

    bt_stream_volume = vol;
	if (!app_bt_stream_isrun(APP_PLAY_BACK_AUDIO)){
	    af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
	    stream_cfg->vol = vol;
	    af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
	}
    return 0;
}

int app_bt_stream_volumeget(void)
{
    return bt_stream_volume;
}

bool app_bt_stream_isrun(APP_BT_STREAM_T player)
{

    if (gStreamplayer == player)
        return true;
    else
        return false;
}

int app_bt_stream_closeall()
{
    TRACE("app_bt_stream_closeall");

    bt_sco_player(false, APP_SYSFREQ_32K);
    bt_sbc_player(PLAYER_OPER_STOP, APP_SYSFREQ_32K);

    #ifdef MEDIA_PLAYER_SUPPORT
    app_play_audio_onoff(false, NULL);
    #endif
    gStreamplayer = APP_BT_STREAM_NUM;
    return 0;
}

bool app_bt_stream_work_status(void) {
    return (gStreamplayer != APP_BT_STREAM_NUM) ;
}

void app_bt_play_warning_tone(void ) {
#ifdef MEDIA_PLAYER_SUPPORT
		media_PlayAudio(AUD_ID_BT_WARNING,0);
#endif

}
