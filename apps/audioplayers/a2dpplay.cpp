#include "mbed.h"
// Standard C Included Files
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "rtos.h"
#include "tgt_hardware.h"
#include "SDFileSystem.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "audioflinger.h"
#include "cqueue.h"
#include "app_audio.h"
#include "analog.h"

// BT
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
#ifdef EQ_PROCESS
#include "eq_export.h"
#endif
enum A2DPPLAY_STRTEAM_T {
    A2DPPLAY_STRTEAM_PUT= 0,
    A2DPPLAY_STRTEAM_GET,
    A2DPPLAY_STRTEAM_QTY,
};

/* sbc decoder */
static bool need_init_decoder = true;
static SbcDecoder* sbc_decoder = NULL;

#define A2DP_AUDIO_SYNC_WITH_LOCAL (1)
#ifdef A2DP_AUDIO_SYNC_WITH_LOCAL
#define A2DP_AUDIO_SYNC_WITH_LOCAL_STEP (2)
#define A2DPPLAY_SYNC_STATUS_SET (0x01)
#define A2DPPLAY_SYNC_STATUS_RESET (0x02)
#define A2DPPLAY_SYNC_STATUS_PROC (0x04)
#endif

#if FPGA==1
#define CFG_HW_AUD_EQ_NUM_BANDS (8)
const int8_t cfg_aud_eq_sbc_band_settings[CFG_HW_AUD_EQ_NUM_BANDS] = {0, 0, 0, 0, 0, 0, 0, 0};
#endif
static float sbc_eq_band_gain[CFG_HW_AUD_EQ_NUM_BANDS];

/* sbc queue */
#define SBC_TEMP_BUFFER_SIZE 128
#define SBC_QUEUE_SIZE_DEFAULT (SBC_TEMP_BUFFER_SIZE*64)
CQueue sbc_queue;

static uint16_t g_sbc_queue_size = SBC_QUEUE_SIZE_DEFAULT;
static int16_t sbc_frame_size = SBC_TEMP_BUFFER_SIZE;
static uint16_t sbc_frame_rev_len = 0;
static uint16_t sbc_frame_num = 0;

static enum APP_AUDIO_CACHE_T a2dp_cache_status = APP_AUDIO_CACHE_QTY;

#define A2DP_SYNC_WITH_GET_MUTUX_TIMEROUT_CNT (5)
#define A2DP_SYNC_WITH_GET_MUTUX_TIMEROUT_MS (20)
static osThreadId a2dp_put_thread_tid = NULL;
static bool a2dp_get_need_sync = false;

#define A2DP_SYNC_WITH_GET_MUTUX_ALLOC() do{ \
                                                if (a2dp_put_thread_tid == NULL){ \
                                                    a2dp_put_thread_tid = osThreadGetId(); \
                                                } \
                                           }while(0)

#define A2DP_SYNC_WITH_GET_MUTUX_FREE() do{ \
                                                a2dp_put_thread_tid = NULL; \
                                          }while(0)

#define A2DP_SYNC_WITH_GET_MUTUX_WAIT() do{ \
                                                a2dp_get_need_sync = true; \
                                                if (a2dp_put_thread_tid){ \
                                                    osSignalClear(a2dp_put_thread_tid, 0x80); \
                                                    osSignalWait(0x80, A2DP_SYNC_WITH_GET_MUTUX_TIMEROUT_MS); \
                                                }\
                                          }while(0)

#define A2DP_SYNC_WITH_GET_MUTUX_SET() do{ \
                                            if (a2dp_get_need_sync){ \
                                                a2dp_get_need_sync = false; \
                                                if (a2dp_put_thread_tid){ \
                                                    osSignalSet(a2dp_put_thread_tid, 0x80); \
                                                } \
                                            } \
                                      }while(0)

//#define A2DP_SYNC_WITH_PUT_MUTUX (1)
#define A2DP_SYNC_WITH_PUT_MUTUX_TIMEROUT_CNT (1)
#define A2DP_SYNC_WITH_PUT_MUTUX_TIMEROUT_MS (3)
static osThreadId a2dp_get_thread_tid = NULL;
static bool a2dp_get_put_sync = false;

#define A2DP_SYNC_WITH_PUT_MUTUX_ALLOC() do{ \
                                                if (a2dp_get_thread_tid == NULL){ \
                                                    a2dp_get_thread_tid = osThreadGetId(); \
                                                } \
                                           }while(0)

#define A2DP_SYNC_WITH_PUT_MUTUX_FREE() do{ \
                                                a2dp_get_thread_tid = NULL; \
                                          }while(0)

#define A2DP_SYNC_WITH_PUT_MUTUX_WAIT() do{ \
                                                a2dp_get_put_sync = true; \
                                                if (a2dp_get_thread_tid){\
                                                    osSignalClear(a2dp_get_thread_tid, 0x80); \
                                                    osSignalWait(0x5, A2DP_SYNC_WITH_PUT_MUTUX_TIMEROUT_MS); \
                                                }\
                                          }while(0)

#define A2DP_SYNC_WITH_PUT_MUTUX_SET() do{ \
                                                if (a2dp_get_put_sync){ \
                                                    a2dp_get_put_sync = false; \
                                                    if (a2dp_get_thread_tid){\
                                                        osSignalSet(a2dp_get_thread_tid, 0x80); \
                                                    }\
                                                } \
                                          }while(0)


int a2dp_audio_sbc_set_frame_info(int rcv_len, int frame_num)
{
    if (sbc_frame_rev_len != rcv_len || sbc_frame_num != frame_num){
        sbc_frame_rev_len = rcv_len;
        sbc_frame_num = frame_num;
        sbc_frame_size = rcv_len/frame_num;
    }

    return 0;
}

#ifdef A2DP_AUDIO_SYNC_WITH_LOCAL
static int a2dp_audio_sync_proc(uint8_t status, int shift)
{
    struct AF_STREAM_CONFIG_T *cfg;
    bool need_shift = false;
    static int cur_shift = 0;
    static int dest_shift = 0;

    LOCK_APP_AUDIO_QUEUE();
    if (status & A2DPPLAY_SYNC_STATUS_RESET){
        cur_shift = 0;
        dest_shift = 0;
    }
    if (status & A2DPPLAY_SYNC_STATUS_SET){
        dest_shift = shift;
    }
    if (cur_shift > dest_shift){
        cur_shift--;
        need_shift = true;
    }
    if (cur_shift<dest_shift){
        cur_shift++;
        need_shift = true;
    }
    if (need_shift){
        TRACE("a2dp_audio_sync_proc shift:%d\n", cur_shift);
        af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &cfg, false);
        switch (cfg->sample_rate) {
            case AUD_SAMPRATE_44100:
                analog_aud_pll_config(AUD_SAMPRATE_44100, cur_shift);
                break;
            case AUD_SAMPRATE_48000:
                analog_aud_pll_config(AUD_SAMPRATE_48000, cur_shift);
                break;
            default:
                break;
        }
    }
    UNLOCK_APP_AUDIO_QUEUE();
    return 0;
}
#endif

bool a2dp_audio_isrunning(enum A2DPPLAY_STRTEAM_T stream, bool run)
{
    static bool stream_running[A2DPPLAY_STRTEAM_QTY] = {false, false};

    if (stream >= A2DPPLAY_STRTEAM_QTY)
        return false;

    stream_running[stream] = run;

    if (stream_running[A2DPPLAY_STRTEAM_PUT] && stream_running[A2DPPLAY_STRTEAM_GET])
        return true;
    else
        return false;
}

int store_sbc_buffer(unsigned char *buf, unsigned int len)
{
    int size;
    int cnt = 0;
    int nRet = 0;

    if (!a2dp_audio_isrunning(A2DPPLAY_STRTEAM_PUT, true))
        return 0;


    switch (a2dp_cache_status) {
        case APP_AUDIO_CACHE_CACHEING:
            LOCK_APP_AUDIO_QUEUE();
            nRet = APP_AUDIO_EnCQueue(&sbc_queue, buf, len);
            size = APP_AUDIO_LengthOfCQueue(&sbc_queue);
            UNLOCK_APP_AUDIO_QUEUE();
            if (size > (g_sbc_queue_size/2)){
                TRACE("cache ok:%d\n", size);
                a2dp_cache_status = APP_AUDIO_CACHE_OK;
            }else if (sbc_frame_size && (size>=(sbc_frame_size<<6))){
                TRACE("cache ok:%d\n", size);
                a2dp_cache_status = APP_AUDIO_CACHE_OK;
            }else{
                TRACE("cache add:%d\n", len);
            }
            if (CQ_ERR == nRet){
                TRACE("cache add overflow\n");
                a2dp_cache_status = APP_AUDIO_CACHE_OK;
            }
            if (a2dp_cache_status == APP_AUDIO_CACHE_OK){
#ifdef __LOCK_AUDIO_THREAD__
                af_unlock_thread();
#endif
                A2DP_SYNC_WITH_GET_MUTUX_ALLOC();
                A2DP_SYNC_WITH_GET_MUTUX_WAIT();
#ifdef __LOCK_AUDIO_THREAD__
                af_lock_thread();
#endif
            }
            break;
        case APP_AUDIO_CACHE_OK:
            do{
                LOCK_APP_AUDIO_QUEUE();
                nRet = APP_AUDIO_EnCQueue(&sbc_queue, buf, len);
//                size = APP_AUDIO_LengthOfCQueue(&sbc_queue);
                UNLOCK_APP_AUDIO_QUEUE();
//                TRACE("cache add:%d %d/%d \n", len, size, g_sbc_queue_size);
                if (CQ_OK == nRet){
                    nRet = 0;
                    break;
                }else{
    //                TRACE("cache flow control:%d\n", cnt);
#ifdef A2DP_AUDIO_SYNC_WITH_LOCAL
                    a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_SET, 0);
#endif
                    nRet = -1;
#ifdef __LOCK_AUDIO_THREAD__
                    af_unlock_thread();
#endif
                    A2DP_SYNC_WITH_GET_MUTUX_ALLOC();
                    A2DP_SYNC_WITH_GET_MUTUX_WAIT();
#ifdef __LOCK_AUDIO_THREAD__
                    af_lock_thread();
#endif
                }
            }while(cnt++<A2DP_SYNC_WITH_GET_MUTUX_TIMEROUT_CNT);
            break;
        case APP_AUDIO_CACHE_QTY:
        default:
            break;
    }
#ifdef A2DP_SYNC_WITH_PUT_MUTUX
    A2DP_SYNC_WITH_PUT_MUTUX_SET();
#endif

    if (nRet){
        TRACE("cache overflow\n");
    }
    return 0;
}

#if 0
int decode_sbc_frame(unsigned char *pcm_buffer, unsigned int pcm_len)
{
    static uint32_t g_preIrqTime = 0;
    uint8_t underflow = 0;
    int32_t stime,etime;

    unsigned char retry = 0;
    int r = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    static SbcPcmData pcm_data;
    XaStatus ret = XA_STATUS_SUCCESS;
    unsigned short byte_decode = 0;


    stime = hal_sys_timer_get();
    pcm_data.data = (unsigned char*)pcm_buffer;

    LOCK_APP_AUDIO_QUEUE();
again:
    if(need_init_decoder) {
        pcm_data.data = (unsigned char*)pcm_buffer;
        pcm_data.dataLen = 0;
        SBC_InitDecoder(&sbc_decoder);
    }

get_again:
    len1 = len2 = 0;
    r = APP_AUDIO_PeekCQueue(&sbc_queue, SBC_TEMP_BUFFER_SIZE, &e1, &len1, &e2, &len2);

    if(r == CQ_ERR || len1 == 0) {
        int size;
        UNLOCK_APP_AUDIO_QUEUE();
        osDelay(2);
        LOCK_APP_AUDIO_QUEUE();
        if (retry++<8){
            goto get_again;
        }else{
            need_init_decoder = true;
            size = APP_AUDIO_LengthOfCQueue(&sbc_queue);
            underflow = 1;
//            a2dp_cache_status = APP_AUDIO_CACHE_CACHEING;
            TRACE("cache underflow size:%d retry:%d\n", size, retry);
            goto exit;
        }
    }
    {
        uint32_t lock = int_lock();
        ret = SBC_DecodeFrames(&sbc_decoder, (unsigned char *)e1, len1, &byte_decode,
        &pcm_data, pcm_len, sbc_eq_band_gain);
        int_unlock(lock);
    }
    if(ret == XA_STATUS_CONTINUE) {
        need_init_decoder = false;
        APP_AUDIO_DeCQueue(&sbc_queue, 0, len1);
        goto again;

        /* back again */
    }
    else if(ret == XA_STATUS_SUCCESS) {
        need_init_decoder = false;
        r = pcm_data.dataLen;
        pcm_data.dataLen = 0;

        APP_AUDIO_DeCQueue(&sbc_queue, 0, byte_decode);

        //TRACE("p %d\n", pcm_data.sampleFreq);

        /* leave */
    }
    else if(ret == XA_STATUS_FAILED) {
        need_init_decoder = true;
        r = pcm_data.dataLen;
        TRACE("err\n");

        APP_AUDIO_DeCQueue(&sbc_queue, 0, byte_decode);

        /* leave */
    }
    else if(ret == XA_STATUS_NO_RESOURCES) {
        need_init_decoder = false;

        TRACE("no\n");

        /* leav */
        r = 0;
    }

exit:
    etime = hal_sys_timer_get();
    if (underflow || need_init_decoder){
        int32_t a,b;
        a = TICKS_TO_MS(stime - g_preIrqTime);
        b = TICKS_TO_MS(etime - stime);
        TRACE( "sbc_decoder irqDur:%d Decode:%d diff:%d size:%d retry:%d\n ", a, b, (a-b), APP_AUDIO_LengthOfCQueue(&sbc_queue),retry);
    }
    g_preIrqTime = stime;
    UNLOCK_APP_AUDIO_QUEUE();
    return r;
}

#else
int decode_sbc_frame(unsigned char *pcm_buffer, unsigned int pcm_len)
{
    static uint32_t g_preIrqTime = 0;
    uint8_t underflow = 0;
    int32_t stime,etime;

    static unsigned int sbc_next_frame_size;
    unsigned char retry = 0;
    int r = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    unsigned int pcm_offset = 0;
    unsigned int pcm_processed = 0;

    static SbcPcmData pcm_data;
    XaStatus ret = XA_STATUS_SUCCESS;
    unsigned short byte_decode = 0;

    stime = hal_sys_timer_get();

    pcm_data.data = (unsigned char*)pcm_buffer;

    if (!sbc_next_frame_size){
        sbc_next_frame_size = sbc_frame_size;
    }

reinit:
    if(need_init_decoder) {
		sbc_next_frame_size = sbc_frame_size;
		pcm_offset = 0;		
        pcm_data.data = (unsigned char*)(pcm_buffer+pcm_offset);
        pcm_data.dataLen = 0;
        SBC_InitDecoder(sbc_decoder);
    }

get_again:
    LOCK_APP_AUDIO_QUEUE();
    len1 = len2 = 0;
    r = PeekCQueue(&sbc_queue, sbc_next_frame_size, &e1, &len1, &e2, &len2);
    UNLOCK_APP_AUDIO_QUEUE();
#ifdef  __LOCK_AUDIO_THREAD__
    if(r == CQ_ERR) {
        pcm_processed = pcm_data.dataLen;
        TRACE("cache underflow retry:%d\n", retry);
        goto exit;
    }
#elif defined (A2DP_SYNC_WITH_PUT_MUTUX)
    if(r == CQ_ERR) {
        int size;
        A2DP_SYNC_WITH_PUT_MUTUX_ALLOC();
        A2DP_SYNC_WITH_PUT_MUTUX_WAIT();
        if (retry++<A2DP_SYNC_WITH_PUT_MUTUX_TIMEROUT_CNT){
            goto get_again;
        }else{
            LOCK_APP_AUDIO_QUEUE();
            size = LengthOfCQueue(&sbc_queue);
            UNLOCK_APP_AUDIO_QUEUE();
            pcm_processed = pcm_data.dataLen;
            underflow = 1;
//            a2dp_cache_status = APP_AUDIO_CACHE_CACHEING;
            TRACE("cache underflow size:%d retry:%d\n", size, retry);
            goto exit;
        }
    }else{
        retry = 0;
    }
#else
#if 1
    if(r == CQ_ERR) {
        pcm_processed = pcm_data.dataLen;
        TRACE("cache underflow retry:%d\n", retry);
        goto exit;
    }
#else
    if(r == CQ_ERR) {
        int size;
        osDelay(2);
        LOCK_APP_AUDIO_QUEUE();
        size = LengthOfCQueue(&sbc_queue);
        UNLOCK_APP_AUDIO_QUEUE();
        if (retry++<12){
            goto get_again;
        }else{
            pcm_processed = pcm_data.dataLen;
            underflow = 1;
//            a2dp_cache_status = APP_AUDIO_CACHE_CACHEING;
            TRACE("cache underflow size:%d retry:%d\n", size, retry);
            goto exit;
        }
    }
#endif
#endif
    if (!len1){
        TRACE("len1 underflow %d/%d\n", len1, len2);
        goto get_again;
    }
    {
        uint32_t lock = int_lock();
        ret = SBC_DecodeFrames(sbc_decoder, (unsigned char *)e1, len1, &byte_decode,
        &pcm_data, pcm_len-pcm_offset, sbc_eq_band_gain);
        int_unlock(lock);
    }
    if(ret == XA_STATUS_CONTINUE) {
        need_init_decoder = false;
        LOCK_APP_AUDIO_QUEUE();
        DeCQueue(&sbc_queue, 0, byte_decode);
        UNLOCK_APP_AUDIO_QUEUE();

        sbc_next_frame_size = (sbc_frame_size-byte_decode)>0?(sbc_frame_size-byte_decode):sbc_frame_size;

        goto get_again;

        /* back again */
    }
    else if(ret == XA_STATUS_SUCCESS) {
        need_init_decoder = false;
        pcm_processed = pcm_data.dataLen;
        pcm_data.dataLen = 0;

        LOCK_APP_AUDIO_QUEUE();
        DeCQueue(&sbc_queue, 0, byte_decode);
        UNLOCK_APP_AUDIO_QUEUE();

        sbc_next_frame_size = sbc_frame_size;
        //TRACE("p %d\n", pcm_data.sampleFreq);

        /* leave */
    }
    else if(ret == XA_STATUS_FAILED) {
        need_init_decoder = true;
        pcm_processed = pcm_data.dataLen;
        pcm_offset += pcm_data.dataLen;
        TRACE("err mutelen:%d\n",pcm_processed);

        LOCK_APP_AUDIO_QUEUE();
        DeCQueue(&sbc_queue, 0, byte_decode);
        UNLOCK_APP_AUDIO_QUEUE();

        sbc_next_frame_size = (sbc_frame_size-byte_decode)>0?(sbc_frame_size-byte_decode):sbc_frame_size;

        if (pcm_offset<pcm_len)
            goto reinit;
        /* leave */
    }
    else if(ret == XA_STATUS_NO_RESOURCES) {
        need_init_decoder = false;
        pcm_processed = pcm_data.dataLen;
        pcm_offset += pcm_data.dataLen;
        TRACE("no_res mutelen:%d\n",pcm_processed);

        LOCK_APP_AUDIO_QUEUE();
        DeCQueue(&sbc_queue, 0, byte_decode);
        UNLOCK_APP_AUDIO_QUEUE();

        sbc_next_frame_size = (sbc_frame_size-byte_decode)>0?(sbc_frame_size-byte_decode):sbc_frame_size;
    }
exit:

    etime = hal_sys_timer_get();
    if (underflow || need_init_decoder)
        TRACE( "sbc_decoder %d/%d %d\n ", TICKS_TO_MS(stime - g_preIrqTime),TICKS_TO_MS(etime - stime),retry);
    g_preIrqTime = stime;

#ifdef A2DP_AUDIO_SYNC_WITH_LOCAL
    {
        int size;
        LOCK_APP_AUDIO_QUEUE();
        size = LengthOfCQueue(&sbc_queue);
        UNLOCK_APP_AUDIO_QUEUE();
//        TRACE("decode_sbc_frame Queue remain size:%d frame_size:%d\n",size, sbc_frame_size);

        if (size<=(g_sbc_queue_size*2/3)||underflow){
//            TRACE("shift down\n");
            a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_SET, A2DP_AUDIO_SYNC_WITH_LOCAL_STEP);
        }else if(size>=(g_sbc_queue_size*4/5)){
//            TRACE("shift normal\n");
            a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_SET, 0);
        }else{
            a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_PROC, 0);
        }
    }
#endif

    return pcm_processed;
}
#endif
uint32_t a2dp_audio_more_data(uint8_t *buf, uint32_t len)
{
    uint32_t l = 0;

    if (!a2dp_audio_isrunning(A2DPPLAY_STRTEAM_GET, true)){
    }

    if (a2dp_cache_status == APP_AUDIO_CACHE_CACHEING){
#if FPGA==0
        TRACE( "a2dp_audio_more_data cache not ready skip frame");
#endif
        memset(buf, 0, len);
    }else{
        l = decode_sbc_frame(buf, len);
        if (l!=len){
            memset(buf+l, 0, len-l);
            TRACE( "a2dp_audio_more_data decode err %d/%d", l, len);
            a2dp_cache_status = APP_AUDIO_CACHE_CACHEING;
            TRACE( "set to APP_AUDIO_CACHE_CACHEING");
        }
#ifdef EQ_PROCESS
    if (l==len){
        int buffsize = get_process_dsp_buffer_size();
        for (uint8_t i = 0; i < len/buffsize; i++){
            process_dsp(buf+(buffsize*i), NULL, buffsize/4);
        }
    }
#endif
    }

    A2DP_SYNC_WITH_GET_MUTUX_SET();

    return len;
}

int a2dp_audio_init(void)
{
    const float EQLevel[25] = {
        0.0630957,  0.0794328, 0.1,       0.1258925, 0.1584893,
        0.1995262,  0.2511886, 0.3162278, 0.398107 , 0.5011872,
        0.6309573,  0.794328 , 1,         1.258925 , 1.584893 ,
        1.995262 ,  2.5118864, 3.1622776, 3.9810717, 5.011872 ,
        6.309573 ,  7.943282 , 10       , 12.589254, 15.848932
    };//-12~12
    uint8_t *buff = NULL;
    uint8_t i;

    for (i=0; i<sizeof(sbc_eq_band_gain)/sizeof(float); i++){
        sbc_eq_band_gain[i] = EQLevel[cfg_aud_eq_sbc_band_settings[i]+12];
    }

    A2DP_SYNC_WITH_GET_MUTUX_FREE();
    A2DP_SYNC_WITH_PUT_MUTUX_FREE();
    app_audio_mempool_get_buff((uint8_t **)&sbc_decoder, sizeof(SbcDecoder) + 4);

    g_sbc_queue_size = app_audio_mempool_free_buff_size();
    app_audio_mempool_get_buff(&buff, g_sbc_queue_size);
    memset(buff, 0, g_sbc_queue_size);
    a2dp_audio_isrunning(A2DPPLAY_STRTEAM_PUT, false);
    a2dp_audio_isrunning(A2DPPLAY_STRTEAM_GET, false);

    LOCK_APP_AUDIO_QUEUE();
    APP_AUDIO_InitCQueue(&sbc_queue, g_sbc_queue_size, buff);
    UNLOCK_APP_AUDIO_QUEUE();

#ifdef A2DP_AUDIO_SYNC_WITH_LOCAL
    a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_SET|A2DPPLAY_SYNC_STATUS_RESET, A2DP_AUDIO_SYNC_WITH_LOCAL_STEP);
#endif

    a2dp_cache_status = APP_AUDIO_CACHE_CACHEING;
    need_init_decoder =  true;

    sbc_frame_rev_len = 0;
    sbc_frame_num = 0;
    sbc_frame_size = SBC_TEMP_BUFFER_SIZE;

    return 0;
}

int a2dp_audio_deinit(void)
{
    A2DP_SYNC_WITH_GET_MUTUX_SET();
    A2DP_SYNC_WITH_PUT_MUTUX_SET();

    A2DP_SYNC_WITH_GET_MUTUX_FREE();
    A2DP_SYNC_WITH_PUT_MUTUX_FREE();

    a2dp_audio_isrunning(A2DPPLAY_STRTEAM_PUT, false);
    a2dp_audio_isrunning(A2DPPLAY_STRTEAM_GET, false);

    a2dp_cache_status = APP_AUDIO_CACHE_CACHEING;
    need_init_decoder =  true;

    sbc_frame_rev_len = 0;
    sbc_frame_num = 0;
    sbc_frame_size = SBC_TEMP_BUFFER_SIZE;

    return 0;
}

