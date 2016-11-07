//#include "mbed.h"
#include <stdio.h>
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_chipid.h"
#include "analog.h"
#include "app_audio.h"
#include "app_status_ind.h"


extern "C" {
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "a2dp.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "hf.h"
#include "btalloc.h"
}


#include "rtos.h"
#include "iabt.h"

#include "cqueue.h"
#include "btapp.h"
#include "app_bt.h"

#include "nvrecord.h"

#include "apps.h"
#include "resources.h"

#include "app_bt_media_manager.h"

/* hfp */
//HfChannel hf_channel;
void restart_app_10s_timers(void);
void enter_scanmode_status(int voice) ;
extern void app_bt_play_warning_tone(void);
int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len);
int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len);

int store_voicepcm_buffer(unsigned char *buf, unsigned int len);
int store_voicecvsd_buffer(unsigned char *buf, unsigned int len);
int store_voicemsbc_buffer(unsigned char *buf, unsigned int len);

#ifdef __BT_ONE_BRING_TWO__
extern BtDeviceRecord record2_copy;
extern uint8_t record2_avalible;
#endif

struct HF_SENDBUFF_CONTROL_T  hf_sendbuff_ctrl;

#if defined(SCO_LOOP)
#define HF_LOOP_CNT (20)
#define HF_LOOP_SIZE (360)

uint8_t hf_loop_buffer[HF_LOOP_CNT*HF_LOOP_SIZE];
uint32_t hf_loop_buffer_len[HF_LOOP_CNT];
uint32_t hf_loop_buffer_valid = 1;
uint32_t hf_loop_buffer_size = 0;
char hf_loop_buffer_w_idx = 0;
#endif

extern struct BT_DEVICE_T  app_bt_device;

void app_hfp_init(void)
{
    app_bt_device.curr_hf_channel_id = BT_DEVICE_ID_1;
    app_bt_device.hf_mute_flag = 0;

    for(uint8_t i=0; i<BT_DEVICE_NUM; i++)
    {
        app_bt_device.hfchan_call[i] = 0;
        app_bt_device.hfchan_callSetup[i] = 0;
        app_bt_device.hf_audio_state[i] = 0;
        app_bt_device.hf_conn_flag[i] = 0;
        app_bt_device.hf_voice_en[i] = 0;
        app_bt_device.bt_open_recon_flag[i] = 0;
    }
}

#if defined(__BT_RECONNECT__) && defined(__BT_HFP_RECONNECT__)
static struct BT_HF_RECONNECT_T bt_hfp_reconnect;

void reconnect_hfp_service(enum BT_DEVICE_ID_T chan_id)
{
    HfChannel *Chan = NULL;
    TRACE("!!!reconnect_hfp_service\n");
    Chan = bt_hfp_reconnect.copyChan[chan_id];
    HF_CreateServiceLink(Chan,&bt_hfp_reconnect.copyAddr[chan_id]);
//    HF_CreateServiceLink(Chan,&app_bt_device.hf_channel[chan_id].cmgrHandler.remDev->bdAddr);
}

void hfp_reconnect_timer_callback1(void const *n) {
    if(bt_hfp_reconnect.TimerNotifyFunc[BT_DEVICE_ID_1])
        bt_hfp_reconnect.TimerNotifyFunc[BT_DEVICE_ID_1]();
}
osTimerDef(hfp_reconnect_timer1, hfp_reconnect_timer_callback1);

#ifdef __BT_ONE_BRING_TWO__
void hfp_reconnect_timer_callback2(void const *n) {
    if(bt_hfp_reconnect.TimerNotifyFunc[BT_DEVICE_ID_2])
        bt_hfp_reconnect.TimerNotifyFunc[BT_DEVICE_ID_2]();
}
osTimerDef(hfp_reconnect_timer2, hfp_reconnect_timer_callback2);
void hfp_reconnect2(void);
#endif

void cancel_hfp_reconnect(enum BT_DEVICE_ID_T chan_id)
{
    if(bt_hfp_reconnect.TimerNotifyFunc[chan_id]) {
        osTimerStop(bt_hfp_reconnect.TimerID[chan_id]);
        osTimerDelete(bt_hfp_reconnect.TimerID[chan_id]);
        bt_hfp_reconnect.TimerNotifyFunc[chan_id] = 0;
        bt_hfp_reconnect.TimerID[chan_id] = 0;
    }
}

void hfp_reconnect1(void)
{
    TRACE("hfp_reconnect1: \n");
#if 0//def __BT_ONE_BRING_TWO__
    if(bt_hfp_reconnect.TimerID[BT_DEVICE_ID_2] != 0)
    {
        TRACE("hfp_reconnect1: start timer2 again\n");
        cancel_hfp_reconnect(BT_DEVICE_ID_2);
        bt_hfp_reconnect.TimerID[BT_DEVICE_ID_2] = osTimerCreate(osTimer(hfp_reconnect_timer2), osTimerOnce, (void *)0);
        bt_hfp_reconnect.TimerNotifyFunc[BT_DEVICE_ID_2] = hfp_reconnect2;
        osTimerStart(bt_hfp_reconnect.TimerID[BT_DEVICE_ID_2], 30000);
    }
#endif
    reconnect_hfp_service(BT_DEVICE_ID_1);
}

#ifdef __BT_ONE_BRING_TWO__
void hfp_reconnect2(void)
{
    TRACE("hfp_reconnect2: \n");
    if(bt_hfp_reconnect.TimerID[BT_DEVICE_ID_1] != 0)
    {
        TRACE("hfp_reconnect2: start timer1 again\n");
        cancel_hfp_reconnect(BT_DEVICE_ID_1);
        bt_hfp_reconnect.TimerID[BT_DEVICE_ID_1] = osTimerCreate(osTimer(hfp_reconnect_timer1), osTimerOnce, (void *)0);
        bt_hfp_reconnect.TimerNotifyFunc[BT_DEVICE_ID_1] = hfp_reconnect1;
        osTimerStart(bt_hfp_reconnect.TimerID[BT_DEVICE_ID_1], 30000);

     }
     reconnect_hfp_service(BT_DEVICE_ID_2);
}


void hfp_open_reconnect2(void)
{
    cancel_hfp_reconnect(BT_DEVICE_ID_1);
    if(MEC(pendCons) == 0){
        HF_CreateServiceLink(&app_bt_device.hf_channel[BT_DEVICE_ID_2], &record2_copy.bdAddr);
    }
}
#endif

#endif




#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_BATTERY_REPORT
int app_hfp_battery_report(uint8_t level)
{
    // Care: BT_DEVICE_NUM<-->{0xff, 0xff, ...}
#ifdef __BT_ONE_BRING_TWO__
    static uint8_t battery_level[BT_DEVICE_NUM] = {0xff, 0xff};
#else
    static uint8_t battery_level[BT_DEVICE_NUM] = {0xff};
#endif

    uint8_t i;
    int nRet = 0;

    if (level>9)
        return -1;

    for(i=0; i<BT_DEVICE_NUM; i++)
    {
        if((app_bt_device.hf_channel[i].state == HF_STATE_OPEN)){
            if (battery_level[i] != level){
                if (HF_Battery_Report(&app_bt_device.hf_channel[i], &app_bt_device.hf_command[i], level) == BT_STATUS_PENDING){
                    battery_level[i] = level;
                }else{
                    nRet = -1;
                }
            }
        }else{
             battery_level[i] = 0xff;
             nRet = -1;
        }
    }
    return nRet;
}
#endif


#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
int app_hfp_siri_report()
{

    uint8_t i;
    BtStatus res;
    for(i=0; i<BT_DEVICE_NUM; i++)
    {
        if((app_bt_device.hf_channel[i].state == HF_STATE_OPEN))
        {
            res = HF_Siri_Report(&app_bt_device.hf_channel[i], &app_bt_device.hf_command[i]);
            if (res == BT_STATUS_PENDING)
            {
                TRACE("[%s] Line = %d, res = %d", __func__, __LINE__, res);
            }
            else
            {
                TRACE("[%s] Line = %d, res = %d", __func__, __LINE__, res);
            }
        }
    }
    return 0;
}

int app_hfp_siri_voice(bool en)
{

    uint8_t i;
    BtStatus res;
    for(i=0; i<BT_DEVICE_NUM; i++)
    {
        if((app_bt_device.hf_channel[i].state == HF_STATE_OPEN))
        {
            res = HF_EnableVoiceRecognition(&app_bt_device.hf_channel[i], en, &app_bt_device.hf_command[i]);
            if (res == BT_STATUS_PENDING)
            {
                TRACE("[%s] Line = %d, res = %d", __func__, __LINE__, res);
            }
            else
            {
                TRACE("[%s] Line = %d, res = %d", __func__, __LINE__, res);
            }
        }
    }
    return 0;
}
#endif


#if !defined(FPGA) && defined(__EARPHONE__)
void hfp_app_status_indication(enum BT_DEVICE_ID_T chan_id,HfCallbackParms *Info)
{
    switch(Info->event)
    {
        case HF_EVENT_SERVICE_CONNECTED:
            app_voice_report(APP_STATUS_INDICATION_CONNECTED,chan_id);
            app_status_indication_set(APP_STATUS_INDICATION_PAGESCAN);
            break;
        case HF_EVENT_SERVICE_DISCONNECTED:
            app_voice_report(APP_STATUS_INDICATION_DISCONNECTED,chan_id);
            break;
        case HF_EVENT_CURRENT_CALL_STATE:
            TRACE("!!!HF_EVENT_CURRENT_CALL_STATE  chan_id:%d, call_number:%s\n", chan_id,Info->p.callListParms->number);
            if(app_bt_device.hfchan_callSetup[chan_id] == HF_CALL_SETUP_IN){
                //////report incoming call number
                app_status_set_num(Info->p.callListParms->number);
                app_voice_report(APP_STATUS_INDICATION_CALLNUMBER,chan_id);
            }
            break;
        case HF_EVENT_CALL_IND:
            if(Info->p.call == HF_CALL_NONE && app_bt_device.hfchan_call[chan_id] == HF_CALL_ACTIVE){
                //////report call hangup voice
                TRACE("!!!HF_EVENT_CALL_IND  APP_STATUS_INDICATION_HANGUPCALL  chan_id:%d\n",chan_id);
                app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP_MEDIA,BT_STREAM_VOICE,chan_id,0);
                app_voice_report(APP_STATUS_INDICATION_HANGUPCALL,chan_id);
            }
			if(Info->p.call == HF_CALL_ACTIVE ) {
				app_bt_play_warning_tone();//////////////duÒ»Éù
			} 
            break;
        case HF_EVENT_CALLSETUP_IND:
            if(Info->p.callSetup == HF_CALL_SETUP_NONE &&
                (app_bt_device.hfchan_call[chan_id] != HF_CALL_ACTIVE) &&
                (app_bt_device.hfchan_callSetup[chan_id] != HF_CALL_SETUP_NONE)){
                ////check the call refuse and stop media of (ring and call number)
                TRACE("!!!HF_EVENT_CALLSETUP_IND  APP_STATUS_INDICATION_REFUSECALL  chan_id:%d\n",chan_id);
                app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP_MEDIA,BT_STREAM_VOICE,chan_id,0);
                app_voice_report(APP_STATUS_INDICATION_REFUSECALL,chan_id);/////////////duÁ½Éù
#if defined(__1MORE_EB100_STYLE__)
            } else if(Info->p.callSetup == HF_CALL_SETUP_IN &&
                (app_bt_device.hfchan_call[chan_id] != HF_CALL_ACTIVE) &&
                (app_bt_device.hfchan_callSetup[chan_id] == HF_CALL_SETUP_NONE)){
				//app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_SWITCHTO_SCO,BT_STREAM_VOICE,chan_id,0);
			} else { //the third party call waiting

				if(Info->p.callSetup == HF_CALL_SETUP_IN &&
					(app_bt_device.hfchan_call[chan_id] == HF_CALL_ACTIVE) &&
					(app_bt_device.hfchan_callSetup[chan_id] == HF_CALL_SETUP_NONE)){
					app_voice_report(APP_STATUS_INDICATION_INCOMINGCALL,chan_id);
				}
#endif
            }
            break;
        case HF_EVENT_AUDIO_CONNECTED:
            TRACE("!!!HF_EVENT_AUDIO_CONNECTED  APP_STATUS_INDICATION_ANSWERCALL  chan_id:%d\n",chan_id);
            //app_voice_report(APP_STATUS_INDICATION_ANSWERCALL,chan_id);//////////////duÒ»Éù
            break;
        case HF_EVENT_RING_IND:
            app_voice_report(APP_STATUS_INDICATION_INCOMINGCALL,chan_id);
            break;
        default:
            break;
    }
}
#endif


static struct BT_DEVICE_ID_DIFF chan_id_flag;
#ifdef __BT_ONE_BRING_TWO__
void hfp_chan_id_distinguish(HfChannel *Chan)
{
    if(Chan == &app_bt_device.hf_channel[BT_DEVICE_ID_1]){
        chan_id_flag.id = BT_DEVICE_ID_1;
        chan_id_flag.id_other = BT_DEVICE_ID_2;
    }else if(Chan == &app_bt_device.hf_channel[BT_DEVICE_ID_2]){
        chan_id_flag.id = BT_DEVICE_ID_2;
        chan_id_flag.id_other = BT_DEVICE_ID_1;
    }
}
#endif

int app_bt_stream_volumeget(void);

int hfp_volume_get(void)
{
    int vol = app_bt_stream_volumeget()-1;

    if (vol > 15)
        vol = 15;
    if (vol < 0)
        vol = 0;

    return (vol);
}

int hfp_volume_set(int vol)
{
    if (vol > 15)
        vol = 15;
    if (vol < 0)
        vol = 0;

    return (app_bt_stream_volumeset(vol+1));
}

void hfp_callback(HfChannel *Chan, HfCallbackParms *Info)
{

#ifdef __BT_ONE_BRING_TWO__
    hfp_chan_id_distinguish(Chan);
#else
    chan_id_flag.id = BT_DEVICE_ID_1;
#endif

    switch(Info->event)
    {
    case HF_EVENT_SERVICE_CONNECTED:
        TRACE("::HF_EVENT_SERVICE_CONNECTED  Chan_id:%d\n", chan_id_flag.id);
        cancel_hfp_reconnect(chan_id_flag.id);

#if !defined(FPGA) && defined(__EARPHONE__)
        if(Chan->state == HF_STATE_OPEN){
            ////report connected voice
            app_bt_device.hf_conn_flag[chan_id_flag.id] = 1;
            hfp_app_status_indication(chan_id_flag.id,Info);
        }
#endif

        HF_ReportSpeakerVolume(Chan,hfp_volume_get(),&app_bt_device.hf_command[chan_id_flag.id]);
#ifdef __BT_ONE_BRING_TWO__
#if defined(SPEECH_NOISE_SUPPRESS) || defined(SPEECH_ECHO_CANCEL)
        HF_DisableNREC(Chan,&app_bt_device.hf_command[chan_id_flag.id_other]);
#endif
#else
#if defined(SPEECH_NOISE_SUPPRESS) || defined(SPEECH_ECHO_CANCEL)
        HF_DisableNREC(Chan,&app_bt_device.hf_command[1]);
#endif
#endif

#if defined( __EARPHONE__) && defined(__BT_RECONNECT__)
        if(app_bt_device.bt_open_recon_flag[chan_id_flag.id]){
           ////if it's under bt open reconnect situation, go on connecting a2dp
           A2DP_OpenStream(&app_bt_device.a2dp_stream[chan_id_flag.id],
                                &app_bt_device.hf_channel[chan_id_flag.id].cmgrHandler.remDev->bdAddr);
        }
#endif
        if(chan_id_flag.id == BT_DEVICE_ID_1){
#if defined( __EARPHONE__) &&  !defined(FPGA)
            /////There is a device connected, so stop PAIR_TIMER and POWEROFF_TIMER of earphone.
            app_stop_10_second_timer(APP_PAIR_TIMER_ID);
            app_stop_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif
        }
#ifdef __BT_ONE_BRING_TWO__
        ////if a call is active and start bt open reconnect procedure, process the curr_hf_channel_id
        if(app_bt_device.hf_audio_state[chan_id_flag.id_other] == HF_AUDIO_CON){
            app_bt_device.curr_hf_channel_id = chan_id_flag.id_other;
        }else{
            app_bt_device.curr_hf_channel_id = chan_id_flag.id;
        }
#endif
        break;
    case HF_EVENT_AUDIO_DATA_SENT:
//      TRACE("::HF_EVENT_AUDIO_DATA_SENT %d\n", Info->event);
#if defined(SCO_LOOP)
        hf_loop_buffer_valid = 1;
#endif
        break;
    case HF_EVENT_AUDIO_DATA:
    {
#ifdef __BT_ONE_BRING_TWO__
    if(app_bt_device.hf_voice_en[chan_id_flag.id])
    {
#endif

#ifndef _SCO_BTPCM_CHANNEL_
        uint32_t idx = 0;
        if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)){
            store_voicebtpcm_m2p_buffer(Info->p.audioData->data, Info->p.audioData->len);

            idx = hf_sendbuff_ctrl.index%HF_SENDBUFF_MEMPOOL_NUM;
            get_voicebtpcm_p2m_frame(&(hf_sendbuff_ctrl.mempool[idx].buffer[0]), Info->p.audioData->len);
            hf_sendbuff_ctrl.mempool[idx].packet.data = &(hf_sendbuff_ctrl.mempool[idx].buffer[0]);
            hf_sendbuff_ctrl.mempool[idx].packet.dataLen = Info->p.audioData->len;
            hf_sendbuff_ctrl.mempool[idx].packet.flags = BTP_FLAG_NONE;
            if(!app_bt_device.hf_mute_flag){
                HF_SendAudioData(Chan, &hf_sendbuff_ctrl.mempool[idx].packet);
            }
            hf_sendbuff_ctrl.index++;
        }
#endif

#ifdef __BT_ONE_BRING_TWO__
    }
#endif
    }

#if defined(SCO_LOOP)
    memcpy(hf_loop_buffer + hf_loop_buffer_w_idx*HF_LOOP_SIZE, Info->p.audioData->data, Info->p.audioData->len);
    hf_loop_buffer_len[hf_loop_buffer_w_idx] = Info->p.audioData->len;
    hf_loop_buffer_w_idx = (hf_loop_buffer_w_idx+1)%HF_LOOP_CNT;
    ++hf_loop_buffer_size;

    if (hf_loop_buffer_size >= 18 && hf_loop_buffer_valid == 1) {
        hf_loop_buffer_valid = 0;
        idx = hf_loop_buffer_w_idx-17<0?(HF_LOOP_CNT-(17-hf_loop_buffer_w_idx)):hf_loop_buffer_w_idx-17;
        pkt.flags = BTP_FLAG_NONE;
        pkt.dataLen = hf_loop_buffer_len[idx];
        pkt.data = hf_loop_buffer + idx*HF_LOOP_SIZE;
        HF_SendAudioData(Chan, &pkt);
    }
#endif
        break;
    case HF_EVENT_SERVICE_DISCONNECTED:
        TRACE("::HF_EVENT_SERVICE_DISCONNECTED Chan_id:%d, reason=%x\n", chan_id_flag.id,Info->p.remDev->discReason);
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_VOICE,chan_id_flag.id,MAX_RECORD_NUM);

#if defined(__BT_RECONNECT__) && defined(__BT_HFP_RECONNECT__)
        if(app_bt_device.hf_conn_flag[chan_id_flag.id]){
            if(Info->p.remDev->discReason == 0x8){
				app_bt_play_warning_tone();
                /////If the discReason of the device is connection timeout,start procedure of hfp reconnect.
                TRACE("hfp:chan_id:%d, reconnectTimes = %d\n",chan_id_flag.id,bt_hfp_reconnect.Times[chan_id_flag.id]);
                bt_hfp_reconnect.copyChan[chan_id_flag.id] = Chan;
                bt_hfp_reconnect.copyAddr[chan_id_flag.id] = app_bt_device.hf_channel[chan_id_flag.id].cmgrHandler.remDev->bdAddr;
                if(bt_hfp_reconnect.TimerNotifyFunc[chan_id_flag.id] == 0){
                    if(chan_id_flag.id == BT_DEVICE_ID_1){
                        bt_hfp_reconnect.TimerID[chan_id_flag.id] = osTimerCreate(osTimer(hfp_reconnect_timer1), osTimerOnce, (void *)0);
                        bt_hfp_reconnect.TimerNotifyFunc[chan_id_flag.id] = hfp_reconnect1;
                    }
#ifdef __BT_ONE_BRING_TWO__
                    else{
                        bt_hfp_reconnect.TimerID[chan_id_flag.id] = osTimerCreate(osTimer(hfp_reconnect_timer2), osTimerOnce, (void *)0);
                        bt_hfp_reconnect.TimerNotifyFunc[chan_id_flag.id] = hfp_reconnect2;
                    }
#endif
                }
                bt_hfp_reconnect.Times[chan_id_flag.id] = 0;
                osTimerStart(bt_hfp_reconnect.TimerID[chan_id_flag.id], 15000);
                bt_hfp_reconnect.Times[chan_id_flag.id]++;
				hfp_app_status_indication(chan_id_flag.id,Info);
            }
			else if((bt_hfp_reconnect.Times[chan_id_flag.id] == 0)&&(app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_1] == 0))
			{
				hfp_app_status_indication(chan_id_flag.id,Info);
				enter_scanmode_status(1);
           	}
        }
        ////reconnect fail, and start reconnect procedure again
        else if((bt_hfp_reconnect.Times[chan_id_flag.id] >0) && (bt_hfp_reconnect.Times[chan_id_flag.id] <20))
        {
              osTimerStart(bt_hfp_reconnect.TimerID[chan_id_flag.id], 15000);//*(bt_hfp_reconnect.Times[chan_id_flag.id]+1));
              bt_hfp_reconnect.Times[chan_id_flag.id]++;
			  restart_app_10s_timers();
			  hfp_app_status_indication(chan_id_flag.id,Info);
        } else if (bt_hfp_reconnect.Times[chan_id_flag.id] >= 20) {
			enter_scanmode_status(0);
		}
#endif

#if !defined(FPGA) && defined(__EARPHONE__)
        if(app_bt_device.hf_conn_flag[chan_id_flag.id] ){
            ////report device disconnected voice
            app_bt_device.hf_conn_flag[chan_id_flag.id] = 0;
			//app_bt_send_request(APP_BT_REQ_ACCESS_MODE_SET, BT_DEFAULT_ACCESS_MODE_PAIR);
        }
#endif

#if 0//def __BT_ONE_BRING_TWO__
#if defined( __EARPHONE__) && defined(__BT_RECONNECT__)
        if(chan_id_flag.id == BT_DEVICE_ID_1){
            if(app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_1]){
                app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_1] = 0;
                if(record2_avalible)
                {
                    app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_2] = 1;
                    /////If bt open reconnect of device 1 fails, delay 1s and reconnect device 2.
                    if(bt_hfp_reconnect.TimerNotifyFunc[BT_DEVICE_ID_1] == 0){
                        bt_hfp_reconnect.TimerID[BT_DEVICE_ID_1] = osTimerCreate(osTimer(hfp_reconnect_timer1), osTimerOnce, (void *)0);
                    }
                    bt_hfp_reconnect.TimerNotifyFunc[BT_DEVICE_ID_1] = hfp_open_reconnect2;
                    osTimerStart(bt_hfp_reconnect.TimerID[BT_DEVICE_ID_1], 1000);
                }
                else
                {
#ifdef __EARPHONE_STAY_BOTH_SCAN__
                    TRACE("::HF_EVENT_SERVICE_DISCONNECTED-->app_bt_accessmode_set 1\n");
                    if(app_bt_device.hf_conn_flag[BT_DEVICE_ID_1] == 0 && Info->p.remDev->discReason == 0x0)
                    {
                        app_bt_send_request(APP_BT_REQ_ACCESS_MODE_SET, BT_DEFAULT_ACCESS_MODE_PAIR);
                    }
#endif
                }
            }
        }else if(app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_2])
        {
                ////bt open reconnect of device 2 fails
                app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_2] = 0;
#ifdef __EARPHONE_STAY_BOTH_SCAN__
                TRACE("::HF_EVENT_SERVICE_DISCONNECTED-->app_bt_accessmode_set 2\n");
                if(app_bt_device.hf_conn_flag[BT_DEVICE_ID_1] == 0  && Info->p.remDev->discReason == 0x0)
                {
                    app_bt_send_request(APP_BT_REQ_ACCESS_MODE_SET, BT_DEFAULT_ACCESS_MODE_PAIR);
                }
#endif
        }
#endif
#else
#if 0//defined( __EARPHONE__) && defined(__BT_RECONNECT__)
        if(app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_1]){
            app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_1] = 0;
#ifdef __EARPHONE_STAY_BOTH_SCAN__
                TRACE("::HF_EVENT_SERVICE_DISCONNECTED-->app_bt_accessmode_set 1\n");
                if(app_bt_device.hf_conn_flag[BT_DEVICE_ID_1] == 0 && Info->p.remDev->discReason == 0x0)
                {
                   // app_bt_send_request(APP_BT_REQ_ACCESS_MODE_SET, BT_DEFAULT_ACCESS_MODE_PAIR);
					enter_scanmode_status(1);
                }
#endif
        }
#endif
#endif
        break;
    case HF_EVENT_CALL_IND:
        TRACE("::HF_EVENT_CALL_IND  chan_id:%d, call:%d\n",chan_id_flag.id,Info->p.call);
        if(Info->p.call == HF_CALL_ACTIVE)
            /////stop media of (ring and call number) and switch to sco
            app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_SWITCHTO_SCO,BT_STREAM_VOICE,chan_id_flag.id,0);

#if !defined(FPGA) && defined(__EARPHONE__)
        hfp_app_status_indication(chan_id_flag.id,Info);
#endif


        if(Info->p.call == HF_CALL_ACTIVE){
            app_bt_device.curr_hf_channel_id = chan_id_flag.id;
        }
#ifdef __BT_ONE_BRING_TWO__
        else if((Info->p.call == HF_CALL_NONE)&&(app_bt_device.hfchan_call[chan_id_flag.id_other] == HF_CALL_ACTIVE)){
            app_bt_device.curr_hf_channel_id = chan_id_flag.id_other;
        }
#endif
//        TRACE("!!!HF_EVENT_CALL_IND  curr_hf_channel_id:%d\n",app_bt_device.curr_hf_channel_id);
        app_bt_device.hfchan_call[chan_id_flag.id] = Info->p.call;

#if defined( __BT_ONE_BRING_TWO__)
//&& defined(__BT_REAL_ONE_BRING_TWO__)
        if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
        {
            ////a call is active:
            if(app_bt_device.hfchan_call[chan_id_flag.id] == HF_CALL_ACTIVE){
                if(app_bt_device.hf_audio_state[chan_id_flag.id_other] == HF_AUDIO_CON){
                    Me_switch_sco(app_bt_device.hf_channel[chan_id_flag.id].cmgrHandler.scoConnect->scoHciHandle);
                }
                app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_ENABLE;
                app_bt_device.hf_voice_en[chan_id_flag.id_other] = HF_VOICE_DISABLE;
            }else{
                ////a call is hung up:
                ///if one device  setup a sco connect so get the other device's sco state, if both connect mute the earlier one
                if(app_bt_device.hf_audio_state[chan_id_flag.id_other] == HF_AUDIO_CON){
                    app_bt_device.hf_voice_en[chan_id_flag.id_other] = HF_VOICE_ENABLE;
                    app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_DISABLE;
                }
            }
        }
#endif
        break;
    case HF_EVENT_CALLSETUP_IND:
        TRACE("::HF_EVENT_CALLSETUP_IND chan_id:%d, callSetup = %d\n", chan_id_flag.id,Info->p.callSetup);
#if !defined(FPGA) && defined(__EARPHONE__)
        hfp_app_status_indication(chan_id_flag.id,Info);
#endif

#ifdef __BT_ONE_BRING_TWO__
        if((app_bt_device.hfchan_call[chan_id_flag.id_other] == HF_CALL_ACTIVE)||((app_bt_device.hfchan_callSetup[chan_id_flag.id_other] == HF_CALL_SETUP_IN)&&(app_bt_device.hfchan_call[chan_id_flag.id] != HF_CALL_ACTIVE))){
            app_bt_device.curr_hf_channel_id = chan_id_flag.id_other;
        }else{
            app_bt_device.curr_hf_channel_id = chan_id_flag.id;
        }
        TRACE("!!!HF_EVENT_CALLSETUP_IND curr_hf_channel_id:%d\n",app_bt_device.curr_hf_channel_id);
#endif
        app_bt_device.hfchan_callSetup[chan_id_flag.id] = Info->p.callSetup;
        if(app_bt_device.hfchan_callSetup[chan_id_flag.id]== HF_CALL_SETUP_IN){
            HF_ListCurrentCalls(Chan, &app_bt_device.hf_command[chan_id_flag.id]);
        }
        break;
    case HF_EVENT_CURRENT_CALL_STATE:
        TRACE("::HF_EVENT_CURRENT_CALL_STATE  chan_id:%d\n", chan_id_flag.id);
#if !defined(FPGA) && defined(__EARPHONE__)
        hfp_app_status_indication(chan_id_flag.id,Info);
#endif
        break;
    case HF_EVENT_AUDIO_CONNECTED:
        TRACE("::HF_EVENT_AUDIO_CONNECTED  chan_id:%d\n", chan_id_flag.id);
#if !defined(FPGA) && defined(__EARPHONE__)
        hfp_app_status_indication(chan_id_flag.id,Info);
#endif

        app_bt_device.phone_earphone_mark = 0;
        app_bt_device.hf_mute_flag = 0;

        app_bt_device.hf_audio_state[chan_id_flag.id] = HF_AUDIO_CON;
#ifdef __BT_ONE_BRING_TWO__

//#ifndef __BT_REAL_ONE_BRING_TWO__
        if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
        {
            if(app_bt_device.hfchan_call[chan_id_flag.id] == HF_CALL_ACTIVE){
                Me_switch_sco(app_bt_device.hf_channel[chan_id_flag.id].cmgrHandler.scoConnect->scoHciHandle);
                app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_ENABLE;
                app_bt_device.hf_voice_en[chan_id_flag.id_other] = HF_VOICE_DISABLE;
            }else if(app_bt_device.hf_audio_state[chan_id_flag.id_other] == HF_AUDIO_CON){
                app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_DISABLE;
                app_bt_device.hf_voice_en[chan_id_flag.id_other] = HF_VOICE_ENABLE;
            }
        }
        else
        {
            ///if one device  setup a sco connect so get the other device's sco state, if both connect mute the earlier one
            if(app_bt_device.hf_audio_state[chan_id_flag.id_other] == HF_AUDIO_CON){
                app_bt_device.hf_voice_en[chan_id_flag.id_other] = HF_VOICE_DISABLE;
            }
            app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_ENABLE;
        }
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_VOICE,chan_id_flag.id,MAX_RECORD_NUM);

        if(MEC(activeCons) != 2){
            ////A call is active, set BAM_NOT_ACCESSIBLE mode.
            //ME_SetAccessibleMode(BAM_NOT_ACCESSIBLE, NULL);
            app_bt_send_request(APP_BT_REQ_ACCESS_MODE_SET, BAM_NOT_ACCESSIBLE);
        }
#else
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,BT_STREAM_VOICE,BT_DEVICE_ID_1,MAX_RECORD_NUM);
#endif
        break;
    case HF_EVENT_AUDIO_DISCONNECTED:
        TRACE("::HF_EVENT_AUDIO_DISCONNECTED  chan_id:%d\n", chan_id_flag.id);
        if(app_bt_device.hfchan_call[chan_id_flag.id] == HF_CALL_ACTIVE){
            app_bt_device.phone_earphone_mark = 1;
        }

        app_bt_device.hf_audio_state[chan_id_flag.id] = HF_AUDIO_DISCON;
#ifdef __BT_ONE_BRING_TWO__
        if(app_bt_device.hf_channel[chan_id_flag.id_other].state != HF_STATE_OPEN)
            ////one device disconnected, set accessible mode to BAM_CONNECTABLE_ONLY
            //ME_SetAccessibleMode(BAM_CONNECTABLE_ONLY, NULL);
            app_bt_send_request(APP_BT_REQ_ACCESS_MODE_SET, BAM_CONNECTABLE_ONLY);

        TRACE("!!!HF_EVENT_AUDIO_DISCONNECTED  hfchan_call[chan_id_flag.id_other]:%d\n",app_bt_device.hfchan_call[chan_id_flag.id_other]);
        if((app_bt_device.hfchan_call[chan_id_flag.id_other] == HF_CALL_ACTIVE)||(app_bt_device.hfchan_callSetup[chan_id_flag.id_other] == HF_CALL_SETUP_IN)){
            app_bt_device.curr_hf_channel_id = chan_id_flag.id_other;
            TRACE("!!!HF_EVENT_AUDIO_DISCONNECTED  app_bt_device.curr_hf_channel_id:%d\n",app_bt_device.curr_hf_channel_id);
        }else{
            app_bt_device.curr_hf_channel_id = chan_id_flag.id;
        }

        app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_DISABLE;
        if(app_bt_device.hf_audio_state[chan_id_flag.id_other] == HF_AUDIO_CON){
            app_bt_device.hf_voice_en[chan_id_flag.id_other] = HF_VOICE_ENABLE;
            TRACE("chan_id:%d AUDIO_DISCONNECTED, then enable id_other voice",chan_id_flag.id);
        }
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_VOICE,chan_id_flag.id,MAX_RECORD_NUM);
#else
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_VOICE,BT_DEVICE_ID_1,MAX_RECORD_NUM);
#endif
        break;
    case HF_EVENT_RING_IND:
        TRACE("::HF_EVENT_RING_IND  chan_id:%d\n", chan_id_flag.id);
#if !defined(FPGA) && defined(__EARPHONE__)
        if(app_bt_device.hf_audio_state[chan_id_flag.id] != HF_AUDIO_CON)
            hfp_app_status_indication(chan_id_flag.id,Info);
#endif
        break;
    case HF_EVENT_SPEAKER_VOLUME:
        TRACE("::HF_EVENT_SPEAKER_VOLUME  chan_id:%d,speaker gain = %x\n", chan_id_flag.id,Info->p.ptr);
        hfp_volume_set((int)(uint32_t)Info->p.ptr);
        break;

#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
    case HF_EVENT_SIRI_STATUS:
        TRACE("[%s] siri status = %d", __func__, Info->p.ptr);
        break;
#endif
    default:
        break;

    }
}

#ifdef __EARPHONE__
/////////profile safely exit
BtStatus LinkDisconnectDirectly(void)
{
    OS_LockStack();

    if(app_bt_device.hf_channel[BT_DEVICE_ID_1].state == HF_STATE_OPEN)
    {
//        MeDisconnectLink(app_bt_device.hf_channel[BT_DEVICE_ID_1].cmgrHandler.remDev);
        HF_DisconnectServiceLink(&app_bt_device.hf_channel[BT_DEVICE_ID_1]);
    }
    if(app_bt_device.a2dp_stream[BT_DEVICE_ID_1].stream.state == AVDTP_STRM_STATE_STREAMING ||
        app_bt_device.a2dp_stream[BT_DEVICE_ID_1].stream.state == AVDTP_STRM_STATE_OPEN)
    {
//        MeDisconnectLink(app_bt_device.hf_channel[BT_DEVICE_ID_1].cmgrHandler.remDev);
        A2DP_CloseStream(&app_bt_device.a2dp_stream[BT_DEVICE_ID_1]);
    }

#ifdef __BT_ONE_BRING_TWO__

#if 0
    if(app_bt_device.hf_channel[BT_DEVICE_ID_2].state == HF_STATE_OPEN)
    {
        MeDisconnectLink(app_bt_device.hf_channel[BT_DEVICE_ID_2].cmgrHandler.remDev);
    }
#endif
    if(app_bt_device.hf_channel[BT_DEVICE_ID_2].state == HF_STATE_OPEN)
    {
//        MeDisconnectLink(app_bt_device.hf_channel[BT_DEVICE_ID_1].cmgrHandler.remDev);
        HF_DisconnectServiceLink(&app_bt_device.hf_channel[BT_DEVICE_ID_2]);
    }
    if(app_bt_device.a2dp_stream[BT_DEVICE_ID_2].stream.state == AVDTP_STRM_STATE_STREAMING ||
        app_bt_device.a2dp_stream[BT_DEVICE_ID_2].stream.state == AVDTP_STRM_STATE_OPEN)
    {
//        MeDisconnectLink(app_bt_device.hf_channel[BT_DEVICE_ID_1].cmgrHandler.remDev);
        A2DP_CloseStream(&app_bt_device.a2dp_stream[BT_DEVICE_ID_2]);
    }

#endif
    OS_UnlockStack();

    return BT_STATUS_SUCCESS;
}
#endif

uint8_t btapp_hfp_get_call_state(void)
{
    uint8_t i;
    for(i=0;i<BT_DEVICE_NUM;i++)
   {
        if(app_bt_device.hfchan_call[i] == HF_CALL_ACTIVE)
            return 1;
    }
    return 0;
}

bool btapp_hfp_call_state_active(void)
{
    uint8_t i;
    for(i=0;i<BT_DEVICE_NUM;i++)
   {
        if(app_bt_device.hfchan_call[i] != HF_CALL_NONE||app_bt_device.hfchan_callSetup[i] != HF_CALL_SETUP_NONE)
            return true;
    }
    return false;
}

void btapp_hfp_report_speak_gain(void)
{

    uint8_t i;


    for(i=0; i<BT_DEVICE_NUM; i++)
    {
        if((app_bt_device.hf_channel[i].state == HF_STATE_OPEN)){
            HF_ReportSpeakerVolume(&app_bt_device.hf_channel[i],hfp_volume_get(),&app_bt_device.hf_command[i]);
        }
    }


}

bool btapp_hfp_has_connection(void)
{
	if(app_bt_device.hf_conn_flag[BT_DEVICE_ID_1] == 1||app_bt_device.hf_conn_flag[BT_DEVICE_ID_2] ==1)
		return true;
	else
		return false;

}

