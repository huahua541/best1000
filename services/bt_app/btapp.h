#ifndef __BTAPP_H__
#define   __BTAPP_H__

#include "hid.h"
//#define __EARPHONE_STAY_BOTH_SCAN__


/* bt config */
#ifdef _SCO_BTPCM_CHANNEL_
#define SYNC_CONFIG_PATH (0<<8|1<<4|1<<0) /* all links use hci */
#else
#define SYNC_CONFIG_PATH (0<<8|0<<4|0<<0) /* all links use hci */
#endif
#define SYNC_CONFIG_MAX_BUFFER (0) /* (e)sco use Packet size */
#ifdef _CVSD_BYPASS_
#define SYNC_CONFIG_CVSD_BYPASS (1) /* use pcm over hci */
#else
#define SYNC_CONFIG_CVSD_BYPASS (0) /* use pcm over hci */
#endif
#define INQ_EXT_RSP_LEN 240


/////app sec.h
extern BtHandler pair_handler;

void pair_handler_func(const BtEvent* evt);

#if BT_SECURITY == XA_ENABLED
extern BtHandler auth_handler;
void auth_handler_func(const BtEvent* evt);
#endif




///a2dp app include
/* a2dp */
/* Default SBC codec configuration */
/* data type for the SBC Codec Information Element*/
/*****************************************************************************
**  Constants
*****************************************************************************/
/* the length of the SBC Media Payload header. */
#define A2D_SBC_MPL_HDR_LEN         1

/* the LOSC of SBC media codec capabilitiy */
#define A2D_SBC_INFO_LEN            6

/* for Codec Specific Information Element */
#define A2D_SBC_IE_SAMP_FREQ_MSK    0xF0    /* b7-b4 sampling frequency */
#define A2D_SBC_IE_SAMP_FREQ_16     0x80    /* b7:16  kHz */
#define A2D_SBC_IE_SAMP_FREQ_32     0x40    /* b6:32  kHz */
#define A2D_SBC_IE_SAMP_FREQ_44     0x20    /* b5:44.1kHz */
#define A2D_SBC_IE_SAMP_FREQ_48     0x10    /* b4:48  kHz */

#define A2D_SBC_IE_CH_MD_MSK        0x0F    /* b3-b0 channel mode */
#define A2D_SBC_IE_CH_MD_MONO       0x08    /* b3: mono */
#define A2D_SBC_IE_CH_MD_DUAL       0x04    /* b2: dual */
#define A2D_SBC_IE_CH_MD_STEREO     0x02    /* b1: stereo */
#define A2D_SBC_IE_CH_MD_JOINT      0x01    /* b0: joint stereo */

#define A2D_SBC_IE_BLOCKS_MSK       0xF0    /* b7-b4 number of blocks */
#define A2D_SBC_IE_BLOCKS_4         0x80    /* 4 blocks */
#define A2D_SBC_IE_BLOCKS_8         0x40    /* 8 blocks */
#define A2D_SBC_IE_BLOCKS_12        0x20    /* 12blocks */
#define A2D_SBC_IE_BLOCKS_16        0x10    /* 16blocks */

#define A2D_SBC_IE_SUBBAND_MSK      0x0C    /* b3-b2 number of subbands */
#define A2D_SBC_IE_SUBBAND_4        0x08    /* b3: 4 */
#define A2D_SBC_IE_SUBBAND_8        0x04    /* b2: 8 */

#define A2D_SBC_IE_ALLOC_MD_MSK     0x03    /* b1-b0 allocation mode */
#define A2D_SBC_IE_ALLOC_MD_S       0x02    /* b1: SNR */
#define A2D_SBC_IE_ALLOC_MD_L       0x01    /* b0: loundess */

#define A2D_SBC_IE_MIN_BITPOOL      2
#define A2D_SBC_IE_MAX_BITPOOL      250




//extern A2dpStream a2dp_stream;
extern AvdtpCodec a2dp_avdtpcodec;
extern const unsigned char a2dp_codec_elements[];
//extern AvrcpChannel avrcp_channel;
extern enum AUD_SAMPRATE_T a2dp_sample_rate;

void a2dp_callback(A2dpStream *Stream, const A2dpCallbackParms *Info);
void avrcp_init(void);
void avrcp_callback(AvrcpChannel *chnl, const AvrcpCallbackParms *Parms);

#define AVRCP_KEY_NULL                0
#define AVRCP_KEY_STOP                1
#define AVRCP_KEY_PLAY                2
#define AVRCP_KEY_PAUSE               3
#define AVRCP_KEY_FORWARD             4
#define AVRCP_KEY_BACKWARD            5
#define AVRCP_KEY_VOLUME_UP           6
#define AVRCP_KEY_VOLUME_DOWN         7
#define AVRCP_KEY_ANSWER_CALL         8
#define AVRCP_KEY_HANGUP_CALL         9
#define AVRCP_KEY_REDIAL_LAST_CALL   10
#define AVRCP_KEY_CHANGE_TO_PHONE    11
#define AVRCP_KEY_ADD_TO_EARPHONE    12
#define AVRCP_KEY_MUTE                            13
#define AVRCP_KEY_CLEAR_MUTE                     14
#ifdef __BT_ONE_BRING_TWO__
#define AVRCP_KEY_REJECT_THREEWAY               15
#define AVRCP_KEY_HANGUP_CURR_ANSWER_THREEWAY  16
#define AVRCP_KEY_HOLD_CURR_ANSWER_THREEWAY    17
#endif
//3way calls oper
#define AVRCP_KEY_HOLD_AND_ANSWER                    20
#define AVRCP_KEY_HOLD_SWAP_ANSWER                   21
#define AVRCP_KEY_HOLD_REL_INCOMING                     22



///////hfp app include

#define HF_SENDBUFF_SIZE (320)
#define HF_SENDBUFF_MEMPOOL_NUM (2)

struct HF_SENDBUFF_CONTROL_T {
	struct HF_SENDBUFF_MEMPOOL_T {
		BtPacket packet;
		uint8_t buffer[HF_SENDBUFF_SIZE];
	} mempool[HF_SENDBUFF_MEMPOOL_NUM];
	uint8_t index;
};


//extern HfChannel hf_channel;
extern struct HF_SENDBUFF_CONTROL_T  hf_sendbuff_ctrl;

void hfp_callback(HfChannel *Chan, HfCallbackParms *Info);
void btapp_hfp_report_speak_gain(void);

/* HfAudioConnectState */
typedef U8 HfAudioConnectState;

#define HF_AUDIO_CON           1
#define HF_AUDIO_DISCON        0

/* End of HfAudioConnectState */


#define HF_VOICE_DISABLE  XA_DISABLED;
#define HF_VOICE_ENABLE   XA_ENABLED;

enum BT_DEVICE_ID_T{
    BT_DEVICE_ID_1 = 0,
#ifdef __BT_ONE_BRING_TWO__
    BT_DEVICE_ID_2,
#endif
    BT_DEVICE_NUM
};

struct BT_DEVICE_T{
    A2dpStream a2dp_stream[BT_DEVICE_NUM];
    enum BT_DEVICE_ID_T curr_a2dp_stream_id;
    uint8_t a2dp_state[BT_DEVICE_NUM];
    uint8_t a2dp_streamming[BT_DEVICE_NUM];
    uint8_t a2dp_play_pause_flag;
    uint8_t sample_rate[BT_DEVICE_NUM];
    AvrcpChannel avrcp_channel[BT_DEVICE_NUM];
    HfChannel hf_channel[BT_DEVICE_NUM];
    enum BT_DEVICE_ID_T curr_hf_channel_id;
    HfCallSetupState  hfchan_callSetup[BT_DEVICE_NUM];
    HfCallActiveState hfchan_call[BT_DEVICE_NUM];
    HfAudioConnectState hf_audio_state[BT_DEVICE_NUM];
    HfCommand hf_command[2];
#if AVRCP_ADVANCED_CONTROLLER == XA_ENABLED	
    AvrcpAdvancedPdu *avrcp_cmd1[BT_DEVICE_NUM];
    AvrcpAdvancedPdu *avrcp_cmd2[BT_DEVICE_NUM];
    AvrcpAdvancedPdu *avrcp_get_capabilities_rsp[BT_DEVICE_NUM];
    AvrcpAdvancedPdu *avrcp_control_rsp[BT_DEVICE_NUM];
    AvrcpAdvancedPdu *avrcp_notify_rsp[BT_DEVICE_NUM];
    uint8_t volume_report[BT_DEVICE_NUM];
    
#endif
    uint8_t hf_conn_flag[BT_DEVICE_NUM];
    uint8_t hf_voice_en[BT_DEVICE_NUM];
    uint8_t hf_mute_flag;
    uint8_t bt_open_recon_flag[BT_DEVICE_NUM];
    uint8_t phone_earphone_mark;
#if HID_DEVICE == XA_ENABLED
    HidChannel  hid_channel[BT_DEVICE_NUM];
#endif

    
};



struct BT_DEVICE_ID_DIFF{
    enum BT_DEVICE_ID_T id;
#ifdef __BT_ONE_BRING_TWO__
    enum BT_DEVICE_ID_T id_other;
#endif
};





#if  defined(__BT_RECONNECT__)&&defined(__BT_HFP_RECONNECT__)

struct BT_HF_RECONNECT_T{
    u8  Times[BT_DEVICE_NUM];
    osTimerId TimerID[BT_DEVICE_NUM];
    OsTimerNotify TimerNotifyFunc[BT_DEVICE_NUM];
    HfChannel *copyChan[BT_DEVICE_NUM];
    BT_BD_ADDR copyAddr[BT_DEVICE_NUM];
};

#endif





/////app key handle include
void a2dp_handleKey(void);
void hfp_handle_key(void);
void btapp_a2dp_report_speak_gain(void);

#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
#define   BTAPP_FUNC_KEY			APP_KEY_CODE_FN1
#define   BTAPP_VOLUME_UP_KEY		APP_KEY_CODE_FN2
#define   BTAPP_VOLUME_DOWN_KEY		APP_KEY_CODE_FN3
#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
#define   BTAPP_RELEASE_KEY			APP_KEY_CODE_NONE
#endif
#else
#define   BTAPP_FUNC_KEY			APP_KEY_CODE_PWR
#define   BTAPP_VOLUME_UP_KEY		APP_KEY_CODE_FN1
#define   BTAPP_VOLUME_DOWN_KEY		APP_KEY_CODE_FN2
#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
#define   BTAPP_RELEASE_KEY			APP_KEY_CODE_NONE
#endif
#endif
void bt_key_init(void);
void bt_key_send(uint16_t code, uint16_t event);
void bt_key_handle(void);


#endif
