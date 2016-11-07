//#include "mbed.h"
#include <stdio.h>
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "analog.h"
#include "app_bt_stream.h"

extern "C" {
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "a2dp.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "hf.h"
#include "hid.h"
}

#include "rtos.h"
#include "iabt.h"

#include "cqueue.h"
#include "btapp.h"
#include "app_key.h"
#include "app_audio.h"

#include "apps.h"


extern struct BT_DEVICE_T  app_bt_device;
void app_bt_play_warning_tone(void );


//HfCommand hf_command;
//bool hf_mute_flag = 0;
void hfp_handle_key(uint8_t hfp_key)
{
    HfChannel *hf_channel_tmp = NULL;

#ifdef __BT_ONE_BRING_TWO__
    enum BT_DEVICE_ID_T another_device_id =  (app_bt_device.curr_hf_channel_id == BT_DEVICE_ID_1) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
    TRACE("!!!hfp_handle_key curr_hf_channel=%d\n",app_bt_device.curr_hf_channel_id);
    hf_channel_tmp = (app_bt_device.curr_hf_channel_id == BT_DEVICE_ID_1) ? &(app_bt_device.hf_channel[BT_DEVICE_ID_1]) : &(app_bt_device.hf_channel[BT_DEVICE_ID_2]);
	HfCommand *hf_cmd_tmp = &(app_bt_device.hf_command[app_bt_device.curr_hf_channel_id]);
#else
    hf_channel_tmp = &(app_bt_device.hf_channel[BT_DEVICE_ID_1]);
	HfCommand *hf_cmd_tmp = &(app_bt_device.hf_command[BT_DEVICE_ID_1);
#endif
    switch(hfp_key)
    {
        case AVRCP_KEY_ANSWER_CALL:
            ///answer a incomming call
            TRACE("avrcp_key = AVRCP_KEY_ANSWER_CALL\n");
            HF_AnswerCall(hf_channel_tmp,&app_bt_device.hf_command[BT_DEVICE_ID_1]);
            hfp_key = AVRCP_KEY_NULL;
            break;
        case AVRCP_KEY_HANGUP_CALL:
            TRACE("avrcp_key = AVRCP_KEY_HANGUP_CALL\n");
            HF_Hangup(&app_bt_device.hf_channel[app_bt_device.curr_hf_channel_id],&app_bt_device.hf_command[BT_DEVICE_ID_1]);
            hfp_key = AVRCP_KEY_NULL;
            break;
        case AVRCP_KEY_REDIAL_LAST_CALL:
            ///redail the last call
            TRACE("avrcp_key = AVRCP_KEY_REDIAL_LAST_CALL\n");
            HF_Redial(&app_bt_device.hf_channel[app_bt_device.curr_hf_channel_id],&app_bt_device.hf_command[BT_DEVICE_ID_1]);
            hfp_key = AVRCP_KEY_NULL;
            break;
        case AVRCP_KEY_CHANGE_TO_PHONE:
            ///remove sco and voice change to phone
            TRACE("avrcp_key = AVRCP_KEY_CHANGE_TO_PHONE\n");
            HF_DisconnectAudioLink(hf_channel_tmp);
            hfp_key = AVRCP_KEY_NULL;
            break;
        case AVRCP_KEY_ADD_TO_EARPHONE:
            ///add a sco and voice change to earphone
            TRACE("avrcp_key = AVRCP_KEY_ADD_TO_EARPHONE\n");
            HF_CreateAudioLink(hf_channel_tmp);
            hfp_key = AVRCP_KEY_NULL;
            break;
        case AVRCP_KEY_MUTE:
            TRACE("avrcp_key = AVRCP_KEY_MUTE\n");
            app_bt_device.hf_mute_flag = 1;
            break;
        case AVRCP_KEY_CLEAR_MUTE:
            TRACE("avrcp_key = AVRCP_KEY_CLEAR_MUTE\n");
            app_bt_device.hf_mute_flag = 0;
            break;
#ifdef __BT_ONE_BRING_TWO__
        case AVRCP_KEY_REJECT_THREEWAY:////////double click
            TRACE("avrcp_key = AVRCP_KEY_REJECT_THREEWAY\n");
            HF_Hangup(&app_bt_device.hf_channel[another_device_id],&app_bt_device.hf_command[another_device_id]);
            hfp_key = AVRCP_KEY_NULL;
            break;
        case AVRCP_KEY_HANGUP_CURR_ANSWER_THREEWAY://////////click
            TRACE("avrcp_key = AVRCP_KEY_HANGUP_CURR_ANSWER_THREEWAY\n");
//            HF_Hangup(&app_bt_device.hf_channel[another_device_id],&app_bt_device.hf_command[another_device_id]);
//            HF_AnswerCall(&app_bt_device.hf_channel[app_bt_device.curr_hf_channel_id],&app_bt_device.hf_command[app_bt_device.curr_hf_channel_id]);
            HF_Hangup(&app_bt_device.hf_channel[app_bt_device.curr_hf_channel_id],&app_bt_device.hf_command[app_bt_device.curr_hf_channel_id]);
            HF_AnswerCall(&app_bt_device.hf_channel[another_device_id],&app_bt_device.hf_command[another_device_id]);
            hfp_key = AVRCP_KEY_NULL;
            break;
        case AVRCP_KEY_HOLD_CURR_ANSWER_THREEWAY:///////////long press
            TRACE("avrcp_key = AVRCP_KEY_HOLD_CURR_ANSWER_THREEWAY\n");
            HF_AnswerCall(&app_bt_device.hf_channel[app_bt_device.curr_hf_channel_id],&app_bt_device.hf_command[BT_DEVICE_ID_1]);
            hfp_key = AVRCP_KEY_NULL;
            break;
#endif
		case AVRCP_KEY_HOLD_AND_ANSWER:
            TRACE("avrcp_key = AVRCP_KEY_HOLD_AND_ANSWER\n");
			HF_CallHold(hf_channel_tmp,HF_HOLD_HOLD_ACTIVE_CALLS,0,hf_cmd_tmp);
            hfp_key = AVRCP_KEY_NULL;
			break;
		case AVRCP_KEY_HOLD_SWAP_ANSWER:
            TRACE("avrcp_key = AVRCP_KEY_HOLD_SWAP_ANSWER\n");
			HF_CallHold(hf_channel_tmp,HF_HOLD_HOLD_ACTIVE_CALLS,0,hf_cmd_tmp);
            hfp_key = AVRCP_KEY_NULL;
			break;
		case AVRCP_KEY_HOLD_REL_INCOMING:
            TRACE("avrcp_key = AVRCP_KEY_HOLD_REL_INCOMING\n");
			HF_CallHold(hf_channel_tmp,HF_HOLD_RELEASE_HELD_CALLS,0,hf_cmd_tmp);
            hfp_key = AVRCP_KEY_NULL;
			break;
        default :
            break;
    }
}

//bool a2dp_play_pause_flag = 0;
void a2dp_handleKey(uint8_t a2dp_key)
{
    AvrcpChannel *avrcp_channel_tmp = NULL;
    if(app_bt_device.a2dp_state[app_bt_device.curr_a2dp_stream_id] == 0)
        return;

#ifdef __BT_ONE_BRING_TWO__
    TRACE("!!!a2dp_handleKey curr_a2dp_stream_id=%d\n",app_bt_device.curr_a2dp_stream_id);
    avrcp_channel_tmp = (app_bt_device.curr_a2dp_stream_id == BT_DEVICE_ID_1) ? &(app_bt_device.avrcp_channel[BT_DEVICE_ID_1]) : &(app_bt_device.avrcp_channel[BT_DEVICE_ID_2]);
#else
    avrcp_channel_tmp = &app_bt_device.avrcp_channel[BT_DEVICE_ID_1];
#endif
    switch(a2dp_key)
    {
        case AVRCP_KEY_STOP:
            TRACE("avrcp_key = AVRCP_KEY_STOP");
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_STOP,TRUE);
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_STOP,FALSE);
            app_bt_device.a2dp_play_pause_flag = 0;
            break;
        case AVRCP_KEY_PLAY:
            TRACE("avrcp_key = AVRCP_KEY_PLAY");
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_PLAY,TRUE);
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_PLAY,FALSE);
            app_bt_device.a2dp_play_pause_flag = 1;
            break;
        case AVRCP_KEY_PAUSE:
            TRACE("avrcp_key = AVRCP_KEY_PAUSE");
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_PAUSE,TRUE);
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_PAUSE,FALSE);
            app_bt_device.a2dp_play_pause_flag = 0;
            break;
        case AVRCP_KEY_FORWARD:
            TRACE("avrcp_key = AVRCP_KEY_FORWARD");
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_FORWARD,TRUE);
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_FORWARD,FALSE);
            app_bt_device.a2dp_play_pause_flag = 1;
            break;
        case AVRCP_KEY_BACKWARD:
            TRACE("avrcp_key = AVRCP_KEY_BACKWARD");
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_BACKWARD,TRUE);
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_BACKWARD,FALSE);
            app_bt_device.a2dp_play_pause_flag = 1;
            break;
        case AVRCP_KEY_VOLUME_UP:
            TRACE("avrcp_key = AVRCP_KEY_VOLUME_UP");
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_VOLUME_UP,TRUE);
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_VOLUME_UP,FALSE);
            break;
        case AVRCP_KEY_VOLUME_DOWN:
            TRACE("avrcp_key = AVRCP_KEY_VOLUME_DOWN");
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_VOLUME_DOWN,TRUE);
            AVRCP_SetPanelKey(avrcp_channel_tmp,AVRCP_POP_VOLUME_DOWN,FALSE);
            break;
        default :
            break;
    }
}

//uint8_t phone_earphone_mark = 0;

#if 0
//uint8_t phone_earphone_mark = 0;
uint8_t iic_mask=0;

void btapp_change_to_iic(void)
{
    const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_iic[] = {
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_I2C_SCL, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_I2C_SDA, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };

    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_iic, sizeof(pinmux_iic)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
}

void btapp_change_to_uart0(void)
{
    const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_uart0[] = {
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_UART0_RX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_UART0_TX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };


    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_uart0, sizeof(pinmux_uart0)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
}
#endif

#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
extern int app_hfp_siri_report();
extern int app_hfp_siri_voice(bool en);
int open_siri_flag = 0;

void bt_key_handle_siri_key(enum APP_KEY_EVENT_T event)
{
    switch(event)
    {
        case  APP_KEY_EVENT_NONE:
			if(open_siri_flag == 1){
				TRACE("open siri");
				app_hfp_siri_voice(true);
				open_siri_flag = 0;
			} else {
				app_hfp_siri_voice(false);
			}
            break;
        case  APP_KEY_EVENT_LONGLONGPRESS:
	    case  APP_KEY_EVENT_UP:
			TRACE("long long/up/click event close siri");
            app_hfp_siri_voice(false);
            break;
        default:
            TRACE("unregister down key event=%x",event);
            break;
    }
}
#endif

void bt_key_handle_func_key(enum APP_KEY_EVENT_T event)
{
    static bool have_hold_call = false;
    switch(event)
    {
        case  APP_KEY_EVENT_UP:
        case  APP_KEY_EVENT_CLICK:
#if 0            
            if(iic_mask ==0)
            {
                iic_mask=1;
                btapp_change_to_iic();
                
            }
            else
            {
                iic_mask=0;
                btapp_change_to_uart0();
            }         
#endif            
            TRACE("!!!APP_KEY_EVENT_CLICK  callSetup1&2:%d,%d, call1&2:%d,%d, audio_state1&2:%d,%d\n",app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1],app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2],app_bt_device.hfchan_call[BT_DEVICE_ID_1],app_bt_device.hfchan_call[BT_DEVICE_ID_2],app_bt_device.hf_audio_state[BT_DEVICE_ID_1],app_bt_device.hf_audio_state[BT_DEVICE_ID_2]);
            if((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_NONE)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_NONE)&&(app_bt_device.hf_audio_state[BT_DEVICE_ID_1] == HF_AUDIO_DISCON)&&
                (app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_NONE)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_NONE)&&(app_bt_device.hf_audio_state[BT_DEVICE_ID_2] == HF_AUDIO_DISCON)){
                if(app_bt_device.a2dp_play_pause_flag == 0){
                    a2dp_handleKey(AVRCP_KEY_PLAY);
                }else{
                    a2dp_handleKey(AVRCP_KEY_PAUSE);
                }
            }

            if(((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_IN)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_NONE)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_NONE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] != HF_CALL_SETUP_ALERT))||
                ((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_IN)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_NONE)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_NONE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] != HF_CALL_SETUP_ALERT))){
				app_bt_play_warning_tone( );
				have_hold_call = false;
				hfp_handle_key(AVRCP_KEY_ANSWER_CALL);
            }

            if((((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_OUT)||(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_ALERT))&&(app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_NONE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_NONE))||
                (((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_OUT)||(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_ALERT))&&(app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_NONE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_NONE))){
                TRACE("!!!!call out hangup\n");				
				app_bt_play_warning_tone( );
				have_hold_call = false;
                hfp_handle_key(AVRCP_KEY_HANGUP_CALL);
            }

            if(app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_ACTIVE){
				if(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_IN||have_hold_call){
                    hfp_handle_key(AVRCP_KEY_HOLD_AND_ANSWER);
					have_hold_call = true;
				}
				else if((app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_NONE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_NONE)){
                    hfp_handle_key(AVRCP_KEY_HANGUP_CALL);
					have_hold_call = false;
					app_bt_play_warning_tone( );
                }else if(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_IN||have_hold_call){
                    hfp_handle_key(AVRCP_KEY_HOLD_AND_ANSWER);
					have_hold_call = true;
                }
            }else if(app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_ACTIVE){
                if(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_IN||have_hold_call){
                    hfp_handle_key(AVRCP_KEY_HOLD_AND_ANSWER);
					have_hold_call = true;
				}
                else if((app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_NONE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_NONE)){
                    hfp_handle_key(AVRCP_KEY_HANGUP_CALL);
					app_bt_play_warning_tone( );
					have_hold_call = false;
                }else if(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_IN||have_hold_call){
                    hfp_handle_key(AVRCP_KEY_HOLD_AND_ANSWER);
					have_hold_call = true;
                }
            }

            if((app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_ACTIVE)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_ACTIVE)){
                TRACE("!!!two call both active\n");
                //hfp_handle_key(AVRCP_KEY_HANGUP_CALL);
            }
            break;
        case  APP_KEY_EVENT_DOUBLECLICK:
            if(((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_NONE)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_NONE)&&(app_bt_device.hf_channel[BT_DEVICE_ID_2].state == HF_STATE_CLOSED))||
                ((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_NONE)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_NONE)&&(app_bt_device.hf_channel[BT_DEVICE_ID_1].state == HF_STATE_CLOSED))){
                hfp_handle_key(AVRCP_KEY_REDIAL_LAST_CALL);
            }

            if(((app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_ACTIVE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_NONE))||
                ((app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_ACTIVE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_NONE))){
                if(app_bt_device.phone_earphone_mark == 0){
                    hfp_handle_key(AVRCP_KEY_CHANGE_TO_PHONE);
                }else if(app_bt_device.phone_earphone_mark == 1){
                    hfp_handle_key(AVRCP_KEY_ADD_TO_EARPHONE);
                }
            }

            if(((app_bt_device.hf_audio_state[BT_DEVICE_ID_1] == HF_AUDIO_CON)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_NONE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_NONE))||
                ((app_bt_device.hf_audio_state[BT_DEVICE_ID_2] == HF_AUDIO_CON)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_NONE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_NONE))){
                if(app_bt_device.hf_mute_flag == 0){
                    hfp_handle_key(AVRCP_KEY_MUTE);
                }else{
                    hfp_handle_key(AVRCP_KEY_CLEAR_MUTE);
                }
            }

            if(((app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_ACTIVE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_IN))||
                ((app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_ACTIVE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_IN))){
                hfp_handle_key(AVRCP_KEY_REJECT_THREEWAY);
            }

            break;
        case  APP_KEY_EVENT_LONGPRESS:

			if(((app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_ACTIVE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_IN))||
				((app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_ACTIVE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_IN))){

			   // hfp_handle_key(AVRCP_KEY_ANSWER_CALL);
				hfp_handle_key(AVRCP_KEY_HOLD_REL_INCOMING);
			   have_hold_call = false;
			}
			else if((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] != HF_CALL_SETUP_NONE)||(app_bt_device.hfchan_call[BT_DEVICE_ID_1] != HF_CALL_NONE)
				||(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] != HF_CALL_SETUP_NONE)||(app_bt_device.hfchan_call[BT_DEVICE_ID_1] != HF_CALL_NONE))
			{
				hfp_handle_key(AVRCP_KEY_HANGUP_CALL);
				have_hold_call = false;
			}
#if HID_DEVICE == XA_ENABLED
            if(((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_NONE)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_NONE)&&(app_bt_device.hf_channel[BT_DEVICE_ID_2].state == HF_STATE_CLOSED))||
                ((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_NONE)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_NONE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_NONE)&&(app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_NONE))){

                Hid_Send_capture(&app_bt_device.hid_channel[BT_DEVICE_ID_1]);
                Hid_Send_capture(&app_bt_device.hid_channel[BT_DEVICE_ID_2]);
            }
#endif
            
 #if 0
            if((app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_ACTIVE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_2] == HF_CALL_SETUP_IN)){
                hfp_handle_key(AVRCP_KEY_HOLD_CURR_ANSWER_THREEWAY);
            }else if((app_bt_device.hfchan_call[BT_DEVICE_ID_2] == HF_CALL_ACTIVE)&&(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_IN)){
                hfp_handle_key(AVRCP_KEY_HOLD_CURR_ANSWER_THREEWAY);
            }
#endif
			// app_bt_play_warning_tone( );

            break;
        default:
            TRACE("unregister func key event=%x",event);
            break;
    }
}

#if defined(__APP_KEY_FN_STYLE_A__)
void bt_key_handle_up_key(enum APP_KEY_EVENT_T event)
{
    switch(event)
    {
        case  APP_KEY_EVENT_UP:
        case  APP_KEY_EVENT_CLICK:
//            a2dp_handleKey(AVRCP_KEY_VOLUME_UP);
            app_bt_stream_volumeup();
            btapp_hfp_report_speak_gain();
            btapp_a2dp_report_speak_gain();
            break;
        case  APP_KEY_EVENT_LONGPRESS:
            a2dp_handleKey(AVRCP_KEY_BACKWARD);
            break;
        default:
            TRACE("unregister up key event=%x",event);
            break;
    }
}


void bt_key_handle_down_key(enum APP_KEY_EVENT_T event)
{
    switch(event)
    {
        case  APP_KEY_EVENT_UP:
        case  APP_KEY_EVENT_CLICK:
 //           a2dp_handleKey(AVRCP_KEY_VOLUME_DOWN);
            app_bt_stream_volumedown();
            btapp_hfp_report_speak_gain();
            btapp_a2dp_report_speak_gain();
            break;
        case  APP_KEY_EVENT_LONGPRESS:
            a2dp_handleKey(AVRCP_KEY_FORWARD);

            break;
        default:
            TRACE("unregister down key event=%x",event);
            break;
    }
}
#else //#elif defined(__APP_KEY_FN_STYLE_B__)
void bt_key_handle_up_key(enum APP_KEY_EVENT_T event)
{
    switch(event)
    {
        case  APP_KEY_EVENT_REPEAT:
//            a2dp_handleKey(AVRCP_KEY_VOLUME_UP);
            app_bt_stream_volumeup();
            btapp_hfp_report_speak_gain();
            btapp_a2dp_report_speak_gain();
            break;
        case  APP_KEY_EVENT_UP:
        case  APP_KEY_EVENT_CLICK:
            a2dp_handleKey(AVRCP_KEY_BACKWARD);
            break;
        default:
            TRACE("unregister up key event=%x",event);
            break;
    }
}


void bt_key_handle_down_key(enum APP_KEY_EVENT_T event)
{
    switch(event)
    {
        case  APP_KEY_EVENT_REPEAT:
 //           a2dp_handleKey(AVRCP_KEY_VOLUME_DOWN);
            app_bt_stream_volumedown();
            btapp_hfp_report_speak_gain();
            btapp_a2dp_report_speak_gain();
            break;
        case  APP_KEY_EVENT_UP:
        case  APP_KEY_EVENT_CLICK:
            a2dp_handleKey(AVRCP_KEY_FORWARD);
            break;
        default:
            TRACE("unregister down key event=%x",event);
            break;
    }
}
#endif


APP_KEY_STATUS bt_key;

void bt_key_init(void)
{
    bt_key.code = 0xff;
    bt_key.event = 0xff;
}

void bt_key_send(uint16_t code, uint16_t event)
{
    OS_LockStack();
    if(bt_key.code == 0xff){
        TRACE("%s code:%d evt:%d",__func__, code, event);
        bt_key.code = code;
        bt_key.event = event;
        OS_NotifyEvm();
    }else{
        TRACE("%s busy",__func__);
    }
    OS_UnlockStack();
}

void bt_key_handle(void)
{
    OS_LockStack();
    if(bt_key.code != 0xff)
    {
        TRACE("%s code:%d evt:%d",__func__, bt_key.code, bt_key.event);
        switch(bt_key.code)
        {
            case BTAPP_FUNC_KEY:
                bt_key_handle_func_key((enum APP_KEY_EVENT_T)bt_key.event);
                break;
            case BTAPP_VOLUME_UP_KEY:
                bt_key_handle_up_key((enum APP_KEY_EVENT_T)bt_key.event);
                break;
            case BTAPP_VOLUME_DOWN_KEY:
                bt_key_handle_down_key((enum APP_KEY_EVENT_T)bt_key.event);
                break;
#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
			case BTAPP_RELEASE_KEY:
				bt_key_handle_siri_key((enum APP_KEY_EVENT_T)bt_key.event);
				break;
#endif				
            default:
                TRACE("bt_key_handle  undefined key");
                break;
        }
        bt_key.code = 0xff;
    }    
    OS_UnlockStack();
}
