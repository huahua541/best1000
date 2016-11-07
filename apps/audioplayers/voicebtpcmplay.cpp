#include "mbed.h"
// Standard C Included Files
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "rtos.h"
#include "SDFileSystem.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "cqueue.h"
#include "app_audio.h"
#include "tgt_hardware.h"

// BT
extern "C" {
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "hf.h"
#include "plc.h"
#ifdef  VOICE_DETECT
#include "webrtc_vad_ext.h"
#endif
}

//#define SPEECH_PLC



#define VOICEBTPCM_TRACE(s,...)
//TRACE(s, ##__VA_ARGS__)

/* voicebtpcm_pcm queue */
#define VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH (120)
#define VOICEBTPCM_PCM_TEMP_BUFFER_SIZE (VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH*2)
#define VOICEBTPCM_PCM_QUEUE_SIZE (VOICEBTPCM_PCM_TEMP_BUFFER_SIZE*4)
CQueue voicebtpcm_p2m_queue;
CQueue voicebtpcm_m2p_queue;

static enum APP_AUDIO_CACHE_T voicebtpcm_cache_m2p_status = APP_AUDIO_CACHE_QTY;
static enum APP_AUDIO_CACHE_T voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_QTY;

#ifdef VOICE_DETECT
#define VAD_SIZE (VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH*2)
#define VAD_CNT_MAX (30)
#define VAD_CNT_MIN (-10)
#define VAD_CNT_INCREMENT (1)
#define VAD_CNT_DECREMENT (1)

static VadInst  *g_vadmic = NULL;
static uint16_t *vad_buff;
static uint16_t vad_buf_size = 0;
static int vdt_cnt = 0;
#endif

static void copy_one_trace_to_two_track_16bits(uint16_t *src_buf, uint16_t *dst_buf, uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; ++i) {
        //dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = ((unsigned short)(src_buf[i])<<1);
        dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = (src_buf[i]);
    }
}
void merge_two_trace_to_one_track_16bits(uint8_t chnlsel, uint16_t *src_buf, uint16_t *dst_buf,  uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; i+=2) {
        dst_buf[i/2] = src_buf[i + chnlsel];
    }
}

//playback flow
//bt-->store_voicebtpcm_m2p_buffer-->decode_voicebtpcm_m2p_frame-->audioflinger playback-->speaker
//used by playback, store data from bt to memory
int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len)
{
    int size;
    unsigned int avail_size;

    LOCK_APP_AUDIO_QUEUE();
    avail_size = APP_AUDIO_AvailableOfCQueue(&voicebtpcm_m2p_queue);
    if (len <= avail_size){
        APP_AUDIO_EnCQueue(&voicebtpcm_m2p_queue, buf, len);
    }else{
        VOICEBTPCM_TRACE( "spk buff overflow %d/%d", len, avail_size);
        APP_AUDIO_DeCQueue(&voicebtpcm_m2p_queue, 0, len - avail_size);
        APP_AUDIO_EnCQueue(&voicebtpcm_m2p_queue, buf, len);
    }
    size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_m2p_queue);
    UNLOCK_APP_AUDIO_QUEUE();

    if (size > (VOICEBTPCM_PCM_QUEUE_SIZE/2)) {
        voicebtpcm_cache_m2p_status = APP_AUDIO_CACHE_OK;
    }

    VOICEBTPCM_TRACE("m2p :%d/%d", len, size);

    return 0;
}

//used by playback, decode data from memory to speaker
int decode_voicebtpcm_m2p_frame(unsigned char *pcm_buffer, unsigned int pcm_len)
{
    int r = 0, got_len = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    LOCK_APP_AUDIO_QUEUE();
    r = APP_AUDIO_PeekCQueue(&voicebtpcm_m2p_queue, pcm_len - got_len, &e1, &len1, &e2, &len2);
    UNLOCK_APP_AUDIO_QUEUE();

    if (r){
        VOICEBTPCM_TRACE( "spk buff underflow");
    }

    if(r == CQ_OK) {
        //memcpy(pcm_buffer + got_len, e1, len1);
        if (len1){
//            copy_one_trace_to_two_track_16bits((uint16_t *)e1, (uint16_t *)(pcm_buffer + got_len), len1/2);
            memcpy(pcm_buffer, e1, len1);
            LOCK_APP_AUDIO_QUEUE();
            APP_AUDIO_DeCQueue(&voicebtpcm_m2p_queue, 0, len1);
            UNLOCK_APP_AUDIO_QUEUE();
            got_len += len1;
        }
        if (len2) {
            //memcpy(pcm_buffer + got_len, e2, len2);
//            copy_one_trace_to_two_track_16bits((uint16_t *)e2, (uint16_t *)(pcm_buffer + got_len), len2/2);
            memcpy(pcm_buffer + len1, e2, len2);
            LOCK_APP_AUDIO_QUEUE();
            APP_AUDIO_DeCQueue(&voicebtpcm_m2p_queue, 0, len2);
            UNLOCK_APP_AUDIO_QUEUE();
            got_len += len2;
        }
    }

    return got_len;
}

//capture flow
//mic-->audioflinger capture-->store_voicebtpcm_p2m_buffer-->get_voicebtpcm_p2m_frame-->bt
//used by capture, store data from mic to memory
int store_voicebtpcm_p2m_buffer(unsigned char *buf, unsigned int len)
{
    int size;
    unsigned int avail_size = 0;
    LOCK_APP_AUDIO_QUEUE();
//    merge_two_trace_to_one_track_16bits(0, (uint16_t *)buf, (uint16_t *)buf, len>>1);
//    r = APP_AUDIO_EnCQueue(&voicebtpcm_p2m_queue, buf, len>>1);
    avail_size = APP_AUDIO_AvailableOfCQueue(&voicebtpcm_p2m_queue);
    if (len <= avail_size){
        APP_AUDIO_EnCQueue(&voicebtpcm_p2m_queue, buf, len);
    }else{
        VOICEBTPCM_TRACE( "mic buff overflow %d/%d", len, avail_size);
        APP_AUDIO_DeCQueue(&voicebtpcm_p2m_queue, 0, len - avail_size);
        APP_AUDIO_EnCQueue(&voicebtpcm_p2m_queue, buf, len);
    }
    size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue);
    UNLOCK_APP_AUDIO_QUEUE();

    VOICEBTPCM_TRACE("p2m :%d/%d", len, size);

    return 0;
}

//used by capture, get the memory data which has be stored by store_voicebtpcm_p2m_buffer()
int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len)
{
    int r = 0, got_len = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
//    int size;

    if (voicebtpcm_cache_p2m_status == APP_AUDIO_CACHE_CACHEING){
        memset(buf, 0, len);
        got_len = len;
    }else{
        LOCK_APP_AUDIO_QUEUE();
//        size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue);
        r = APP_AUDIO_PeekCQueue(&voicebtpcm_p2m_queue, len - got_len, &e1, &len1, &e2, &len2);
        UNLOCK_APP_AUDIO_QUEUE();

//        VOICEBTPCM_TRACE("p2m :%d/%d", len, APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue));

        if(r == CQ_OK) {
            if (len1){
                memcpy(buf, e1, len1);
                LOCK_APP_AUDIO_QUEUE();
                APP_AUDIO_DeCQueue(&voicebtpcm_p2m_queue, 0, len1);
                UNLOCK_APP_AUDIO_QUEUE();
                got_len += len1;
            }
            if (len2 != 0) {
                memcpy(buf+got_len, e2, len2);
                got_len += len2;
                LOCK_APP_AUDIO_QUEUE();
                APP_AUDIO_DeCQueue(&voicebtpcm_p2m_queue, 0, len2);
                UNLOCK_APP_AUDIO_QUEUE();
            }
        }else{
            VOICEBTPCM_TRACE( "mic buff underflow");
            memset(buf, 0, len);
            got_len = len;
            voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_CACHEING;
        }
    }

    return got_len;
}

#if defined (SPEECH_PLC)
static struct PlcSt *speech_lc;
#endif

#if defined( SPEECH_ECHO_CANCEL ) || defined( SPEECH_NOISE_SUPPRESS )

#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"

#define SPEEX_BUFF_SIZE (VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH)

static short *e_buf;

#if defined( SPEECH_ECHO_CANCEL )
static SpeexEchoState *st=NULL;
static short *ref_buf;
static short *echo_buf;
#endif

#if defined( SPEECH_NOISE_SUPPRESS )
static SpeexPreprocessState *den=NULL;
#endif

#endif

#if 0
void get_mic_data_max(short *buf, uint32_t len)
{
    int max0 = -32768, min0 = 32767, diff0 = 0;
    int max1 = -32768, min1 = 32767, diff1 = 0;

    for(uint32_t i=0; i<len/2;i+=2)
    {
        if(buf[i+0]>max0)
        {
            max0 = buf[i+0];
        }

        if(buf[i+0]<min0)
        {
            min0 = buf[i+0];
        }

        if(buf[i+1]>max1)
        {
            max1 = buf[i+1];
        }

        if(buf[i+1]<min1)
        {
            min1 = buf[i+1];
        }
    }
    TRACE("min0 = %d, max0 = %d, diff0 = %d, min1 = %d, max1 = %d, diff1 = %d", min0, max0, max0 - min0, min1, max1, max1 - min1);
}
#endif

#ifdef VOICE_DETECT
int16_t voicebtpcm_vdt(uint8_t *buf, uint32_t len)
{
        int16_t ret = 0;
        uint16_t buf_size = 0;
        uint16_t *p_buf = NULL;
         if(len == 120){
            	vad_buf_size += VAD_SIZE/2; //VAD BUFF IS UINT16
            	if(vad_buf_size < VAD_SIZE){
            		memcpy(vad_buff , buf, VAD_SIZE);
            		return ret;
            	}
                	memcpy(vad_buff + VAD_SIZE/2, buf, VAD_SIZE);
                 p_buf = vad_buff;
                 buf_size = VAD_SIZE;
         }else if(len == 160){
                 p_buf = (uint16_t *)buf;
                 buf_size = VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH;
         }else
                 return 0;

	ret = WebRtcVad_Process((VadInst*)g_vadmic, 8000, (WebRtc_Word16 *)p_buf, buf_size);
	vad_buf_size = 0;
         return ret;
}
#endif	
//used by capture, store data from mic to memory
uint32_t voicebtpcm_pcm_audio_data_come(uint8_t *buf, uint32_t len)
{
	int16_t ret = 0;
	bool vdt = false;
//	int32_t loudness,gain,max1,max2;	
//    uint32_t stime, etime;
//    static uint32_t preIrqTime = 0;

//  VOICEBTPCM_TRACE("%s enter", __func__);
//    stime = hal_sys_timer_get();
    //get_mic_data_max((short *)buf, len);
    int size = 0;
#if defined( SPEECH_ECHO_CANCEL ) || defined( SPEECH_NOISE_SUPPRESS )
    {

 //       int i;
        short *buf_p=(short *)buf;

//        uint32_t lock = int_lock();
#if defined (SPEECH_ECHO_CANCEL)
    //       for(i=0;i<120;i++)
    //       {
    //           ref_buf[i]=buf_p[i];
    //       }
        memcpy(ref_buf, buf_p, SPEEX_BUFF_SIZE<<1);
        speex_echo_cancellation(st, ref_buf, echo_buf, e_buf);
#else
    //        for(i=0;i<120;i++)
    //        {
    //            e_buf[i]=buf_p[i];
    //        }
        memcpy(e_buf, buf_p, SPEEX_BUFF_SIZE<<1);
#endif
#ifdef VOICE_DETECT
    ret = voicebtpcm_vdt((uint8_t*)e_buf,  len);
	if(ret){
	if(vdt_cnt < 0)
		vdt_cnt = 3;
	else
    		vdt_cnt += ret*VAD_CNT_INCREMENT*3;
    }else{
        // not detect voice
    	vdt_cnt -=VAD_CNT_DECREMENT;
    }
	if (vdt_cnt > VAD_CNT_MAX)
		vdt_cnt = VAD_CNT_MAX;
	if (vdt_cnt < VAD_CNT_MIN)
		vdt_cnt = VAD_CNT_MIN;
	if (vdt_cnt > 0)
		vdt =  true;
#endif
#if defined (SPEECH_NOISE_SUPPRESS)
#ifdef VOICE_DETECT
		float gain;
		if (vdt){
			gain = 31.f;
			speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC_FIX_GAIN, &gain);
		}else{
			gain = 1.f;
			speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC_FIX_GAIN, &gain);
		}
#endif	
        speex_preprocess_run(den, e_buf);
#endif
//        int_unlock(lock);
    //        for(i=0;i<120;i++)
    //        {
    //            buf_p[i]=e_buf[i];
    //        }
    //    memcpy(buf_p, e_buf, sizeof(e_buf));






        LOCK_APP_AUDIO_QUEUE();
        store_voicebtpcm_p2m_buffer((unsigned char *)e_buf, SPEEX_BUFF_SIZE<<1);
        size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue);
        UNLOCK_APP_AUDIO_QUEUE();
    }
#else
//       etime = hal_sys_timer_get();
//  VOICEBTPCM_TRACE("\n******total Spend:%d Len:%dn", TICKS_TO_MS(etime - stime), len);
    LOCK_APP_AUDIO_QUEUE();
    store_voicebtpcm_p2m_buffer(buf, len);
    size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue);
    UNLOCK_APP_AUDIO_QUEUE();
#endif

    if (size > (VOICEBTPCM_PCM_QUEUE_SIZE/2)) {
        voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_OK;
    }

//    preIrqTime = stime;

    return len;
}


//used by playback, play data from memory to speaker
uint32_t voicebtpcm_pcm_audio_more_data(uint8_t *buf, uint32_t len)
{
    uint32_t l = 0;
//    uint32_t stime, etime;
//    static uint32_t preIrqTime = 0;

    if (voicebtpcm_cache_m2p_status == APP_AUDIO_CACHE_CACHEING){
        memset(buf, 0, len);
        l = len;
    }else{
#if defined (SPEECH_ECHO_CANCEL)
        {
//            int i;
        short *buf_p=(short *)buf;
//            for(i=0;i<120;i++)
//            {
//                echo_buf[i]=buf_p[i];
//            }
        memcpy(echo_buf, buf_p, SPEEX_BUFF_SIZE<<1);

        }
#endif

/*
         for(int i=0;i<120;i=i+2)
            {
               short *buf_p=(short *)buf;
                TRACE("%5d,%5d,", buf_p[i],buf_p[i+1]);
            hal_sys_timer_delay(2);
            }
*/


// stime = hal_sys_timer_get();
    l = decode_voicebtpcm_m2p_frame(buf, len);
    if (l != len){
        memset(buf, 0, len);
        voicebtpcm_cache_m2p_status = APP_AUDIO_CACHE_CACHEING;
    }

#if defined (SPEECH_PLC)
//stime = hal_sys_timer_get();
    speech_plc(speech_lc,(short *)buf,len);
//etime = hal_sys_timer_get();
//TRACE( "plc cal ticks:%d", etime-stime);
#endif

#ifdef AUDIO_OUTPUT_LR_BALANCE
    app_audio_lr_balance(buf, len, AUDIO_OUTPUT_LR_BALANCE);
#endif


    //    etime = hal_sys_timer_get();
    //  VOICEBTPCM_TRACE("%s irqDur:%03d Spend:%03d Len:%d ok:%d", __func__, TICKS_TO_MS(stime - preIrqTime), TICKS_TO_MS(etime - stime), len>>1, voicebtpcm_cache_m2p_status);

    //    preIrqTime = stime;
    }
    return l;
}

void* speex_get_ext_buff(int size)
{
    uint8_t *pBuff = NULL;
    size = size + (4 - size % 4);
    app_audio_mempool_get_buff(&pBuff, size);
    VOICEBTPCM_TRACE( "speex_get_ext_buff len:%d", size);
    return (void*)pBuff;
}

int voicebtpcm_pcm_audio_init(void)
{
    uint8_t *p2m_buff = NULL;
    uint8_t *m2p_buff = NULL;

    app_audio_mempool_get_buff(&p2m_buff, VOICEBTPCM_PCM_QUEUE_SIZE);
    app_audio_mempool_get_buff(&m2p_buff, VOICEBTPCM_PCM_QUEUE_SIZE);
    memset(p2m_buff, 0, VOICEBTPCM_PCM_QUEUE_SIZE);
    memset(m2p_buff, 0, VOICEBTPCM_PCM_QUEUE_SIZE);

    LOCK_APP_AUDIO_QUEUE();
    /* voicebtpcm_pcm queue*/
    APP_AUDIO_InitCQueue(&voicebtpcm_p2m_queue, VOICEBTPCM_PCM_QUEUE_SIZE, p2m_buff);
    APP_AUDIO_InitCQueue(&voicebtpcm_m2p_queue, VOICEBTPCM_PCM_QUEUE_SIZE, m2p_buff);
    UNLOCK_APP_AUDIO_QUEUE();

    voicebtpcm_cache_m2p_status = APP_AUDIO_CACHE_CACHEING;
    voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_CACHEING;
#if defined( SPEECH_ECHO_CANCEL ) || defined( SPEECH_NOISE_SUPPRESS )
    {
       #define NN VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH
       #define TAIL VOICEBTPCM_SPEEX_8KHZ_FRAME_LENGTH
       int sampleRate = 8000;
       int leak_estimate = 16383;//Better 32767/2^n

       e_buf = (short *)speex_get_ext_buff(SPEEX_BUFF_SIZE<<1);
#if defined( SPEECH_ECHO_CANCEL )
       ref_buf = (short *)speex_get_ext_buff(SPEEX_BUFF_SIZE<<1);
       echo_buf = (short *)speex_get_ext_buff(SPEEX_BUFF_SIZE<<1);
       st = speex_echo_state_init(NN, TAIL, speex_get_ext_buff);
       speex_echo_ctl(st, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);
       speex_echo_ctl(st, SPEEX_ECHO_SET_FIXED_LEAK_ESTIMATE, &leak_estimate);
#endif

#if defined( SPEECH_NOISE_SUPPRESS )
       den = speex_preprocess_state_init(NN, sampleRate, speex_get_ext_buff);
#endif

#if defined( SPEECH_ECHO_CANCEL ) && defined( SPEECH_NOISE_SUPPRESS )
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_ECHO_STATE, st);
#endif
    }
#endif

#if defined( SPEECH_NOISE_SUPPRESS )
    float gain, l;
    spx_int32_t l2;
    spx_int32_t max_gain;
    l= 28000;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC_LEVEL, &l);

    l2 = 0;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC, &l2);

    gain = 0;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC_FIX_GAIN, &gain);

    max_gain = 20;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &max_gain);
    
    l2 = -20;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &l2);

    l2 = -40;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, &l2);

    l2 = -20;
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE, &l2);
#endif


#if defined (SPEECH_PLC)
    speech_lc = speech_plc_init(speex_get_ext_buff);
#endif

#ifdef VOICE_DETECT
    {
    	WebRtcVad_AssignSize(&vdt_cnt);
    	g_vadmic = (VadInst*)speex_get_ext_buff(vdt_cnt + 4);
    	WebRtcVad_Init((VadInst*)g_vadmic);
    	WebRtcVad_set_mode((VadInst*)g_vadmic, 2);
    	//one channel 320*2
    	vad_buff = (uint16_t *)speex_get_ext_buff(VAD_SIZE*2);
    	vad_buf_size = 0;
    	vdt_cnt = 0;
    }    
#endif	
    return 0;
}

