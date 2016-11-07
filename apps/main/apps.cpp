#include "stdio.h"
#include "cmsis_os.h"
#include "string.h"

#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_bootmode.h"

#include "audioflinger.h"
#include "apps.h"
#include "app_thread.h"
#include "app_key.h"
#include "app_pwl.h"
#include "app_audio.h"
#include "app_overlay.h"
#include "app_battery.h"
#include "app_utils.h"
#include "app_status_ind.h"
#ifdef __FACTORY_MODE_SUPPORT__
#include "app_factory.h"
#endif
#include "bt_drv_interface.h"
#include "iabt.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"

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
#include "btalloc.h"
#include "btapp.h"
#include "app_bt.h"

#ifdef MEDIA_PLAYER_SUPPORT
#include "resources.h"
#include "app_media_player.h"
#endif
#include "app_bt_media_manager.h"
#include "codec_best1000.h"

void report_bat_low_status(void);
void enter_scanmode_status(int voice) ;
bool btapp_hfp_call_state_active(void);

bool app_bt_stream_work_status(void) ;

BtStatus LinkDisconnectDirectly(void);


#define APP_BATTERY_LEVEL_LOWPOWERTHRESHOLD (1)


uint8_t  app_poweroff_flag = 0;

static uint8_t app_status_indication_init(void)
{
    struct APP_PWL_CFG_T cfg;
    memset(&cfg, 0, sizeof(struct APP_PWL_CFG_T));
    app_pwl_open();
    app_pwl_setup(APP_PWL_ID_0, &cfg);
    app_pwl_setup(APP_PWL_ID_1, &cfg);
    return 0;
}


#if defined(__EARPHONE__) && defined(__AUTOPOWEROFF__)

void PairingTransferToConnectable(void);

typedef void (*APP_10_SECOND_TIMER_CB_T)(void);

void app_pair_timerout(void);
void app_poweroff_timerout(void);
void CloseEarphone(void);

typedef struct
{
    uint8_t timer_id;
    uint8_t timer_en;
    uint8_t timer_count;
    uint8_t timer_period;
    APP_10_SECOND_TIMER_CB_T cb;
}APP_10_SECOND_TIMER_STRUCT;



APP_10_SECOND_TIMER_STRUCT app_pair_timer =
{
    .timer_id =APP_PAIR_TIMER_ID,
    .timer_en = 0,
    .timer_count = 0,
    .timer_period = 12,
    .cb =    PairingTransferToConnectable
};

APP_10_SECOND_TIMER_STRUCT app_poweroff_timer=
{
    .timer_id =APP_POWEROFF_TIMER_ID,
    .timer_en = 0,
    .timer_count = 0,
    .timer_period = 30,
    .cb =    CloseEarphone
};

APP_10_SECOND_TIMER_STRUCT app_lowpower_timer=
{
    .timer_id =APP_BATLOW_TIMER_ID,
    .timer_en = 0,
    .timer_count = 0,
    .timer_period = 12,
    .cb =    report_bat_low_status
};


APP_10_SECOND_TIMER_STRUCT  *app_10_second_array[3] =
{
    &app_pair_timer,
    &app_poweroff_timer,
    &app_lowpower_timer,
};



void app_stop_10_second_timer(uint8_t timer_id)
{
    APP_10_SECOND_TIMER_STRUCT *timer = app_10_second_array[timer_id];
    timer->timer_en = 0;
    timer->timer_count = 0;
}

void app_start_10_second_timer(uint8_t timer_id)
{
    APP_10_SECOND_TIMER_STRUCT *timer = app_10_second_array[timer_id];

    timer->timer_en = 1;
    timer->timer_count = 0;
}

void app_10_second_timerout_handle(uint8_t timer_id)
{
    APP_10_SECOND_TIMER_STRUCT *timer = app_10_second_array[timer_id];

    timer->timer_en = 0;
    timer->cb();
}

bool app_10_second_timer_pending(uint8_t timer_id)
{
    APP_10_SECOND_TIMER_STRUCT *timer = app_10_second_array[timer_id];
    TRACE("%s timer_id %d\n",__func__,timer_id);
    if(timer->timer_en != 0)
		return true;
    return false;
}


void app_10_second_timer_check(void)
{
    uint8_t i;
    for(i=0;i<sizeof(app_10_second_array)/sizeof(app_10_second_array[0]);i++)
    {
        if(app_10_second_array[i]->timer_en)
        {
            app_10_second_array[i]->timer_count++;
            if(app_10_second_array[i]->timer_count >= app_10_second_array[i]->timer_period)
            {
                app_10_second_timerout_handle(i);
            }
        }

    }
}
#endif

#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_BATTERY_REPORT
extern int app_hfp_battery_report(uint8_t battery_level);
#endif
int app_status_battery_report(uint8_t level)
{
#if defined(__EARPHONE__)
    app_10_second_timer_check();
#endif
    if (level<=APP_BATTERY_LEVEL_LOWPOWERTHRESHOLD){
        //add something
    }
#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_BATTERY_REPORT
    app_hfp_battery_report(level);
#else
    TRACE("[%s] Can not enable HF_CUSTOM_FEATURE_BATTERY_REPORT",__func__);
#endif
    return 0;
}

void app_status_set_num(const char* p)
{
#ifdef MEDIA_PLAYER_SUPPORT
    media_Set_IncomingNumber(p);
#endif
}

extern "C" int app_voice_report(APP_STATUS_INDICATION_T status,uint8_t device_id)
{
#ifdef MEDIA_PLAYER_SUPPORT
    TRACE("%s %d",__func__, status);

    switch (status) {
        case APP_STATUS_INDICATION_POWERON:
            media_PlayAudio(AUD_ID_POWER_ON,0);
            break;
        case APP_STATUS_INDICATION_POWEROFF:
            media_PlayAudio(AUD_ID_POWER_OFF,0);
            break;
        case APP_STATUS_INDICATION_CONNECTED:
            media_PlayAudio(AUD_ID_BT_CONNECTED,device_id);
            break;
        case APP_STATUS_INDICATION_DISCONNECTED:
            if(app_poweroff_flag == 0)
                media_PlayAudio(AUD_ID_BT_DIS_CONNECT,device_id);
            break;
        case APP_STATUS_INDICATION_CALLNUMBER:
            media_PlayAudio(AUD_ID_BT_CALL_INCOMING_NUMBER,device_id);
            break;
        case APP_STATUS_INDICATION_CHARGENEED:
            media_PlayAudio(AUD_ID_BT_CHARGE_PLEASE,0);
            break;
        case APP_STATUS_INDICATION_FULLCHARGE:
            media_PlayAudio(AUD_ID_BT_CHARGE_FINISH,0);
            break;
        case APP_STATUS_INDICATION_PAIRSUCCEED:
            media_PlayAudio(AUD_ID_BT_PAIRING_SUC,device_id);
            break;
        case APP_STATUS_INDICATION_PAIRFAIL:
            media_PlayAudio(AUD_ID_BT_PAIRING_FAIL,device_id);
            break;

        case APP_STATUS_INDICATION_HANGUPCALL:
            media_PlayAudio(AUD_ID_BT_CALL_HUNG_UP,device_id);
            break;

        case APP_STATUS_INDICATION_REFUSECALL:
            media_PlayAudio(AUD_ID_BT_CALL_REFUSE,device_id);
            break;

        case APP_STATUS_INDICATION_ANSWERCALL:
            media_PlayAudio(AUD_ID_BT_CALL_ANSWER,device_id);
            break;

        case APP_STATUS_INDICATION_CLEARSUCCEED:
            media_PlayAudio(AUD_ID_BT_CLEAR_SUCCESS,device_id);
            break;

        case APP_STATUS_INDICATION_CLEARFAIL:
            media_PlayAudio(AUD_ID_BT_CLEAR_FAIL,device_id);
            break;
        case APP_STATUS_INDICATION_INCOMINGCALL:
            media_PlayAudio(AUD_ID_BT_CALL_INCOMING_CALL,device_id);
            break;
        case APP_STATUS_INDICATION_BOTHSCAN:
            if(app_poweroff_flag == 0)
                media_PlayAudio(AUD_ID_BT_PAIR_ENABLE,0);
            break;

		case APP_STATUS_INDICATION_BAT_PWRON_NORMAL:
			if(app_poweroff_flag == 0)
				media_PlayAudio(AUD_ID_BT_BAT_M,0);
			break;
		case APP_STATUS_INDICATION_BAT_PWRON_HIGH:
			if(app_poweroff_flag == 0)
				media_PlayAudio(AUD_ID_BT_BAT_H,0);
			break;
		case APP_STATUS_INDICATION_BAT_PWRON_LOW:
			if(app_poweroff_flag == 0)
				media_PlayAudio(AUD_ID_BT_BAT_L,0);
			break;
		case APP_STATUS_INDICATION_VOL_MAX:
			if(app_poweroff_flag == 0)
				media_PlayAudio(AUD_ID_BT_WARNING,0);
			break;
		case APP_STATUS_INDICATION_VOL_MIN:
			if(app_poweroff_flag == 0)
				media_PlayAudio(AUD_ID_BT_WARNING,0);
			break;

        default:
            break;
    }
#endif
    return 0;
}


#define POWERON_PRESSMAXTIME_THRESHOLD_MS  (10000)

enum APP_POWERON_CASE_T {
    APP_POWERON_CASE_NORMAL = 0,
    APP_POWERON_CASE_DITHERING,
    APP_POWERON_CASE_REBOOT,
    APP_POWERON_CASE_ALARM,
    APP_POWERON_CASE_CALIB,
    APP_POWERON_CASE_BOTHSCAN,
    APP_POWERON_CASE_CHARGING,
    APP_POWERON_CASE_FACTORY,
    APP_POWERON_CASE_TEST,
    APP_POWERON_CASE_INVALID,

    APP_POWERON_CASE_NUM
};


static osThreadId apps_init_tid = NULL;

static enum APP_POWERON_CASE_T pwron_case = APP_POWERON_CASE_INVALID;

static void app_poweron_normal(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    pwron_case = APP_POWERON_CASE_NORMAL;

    osSignalSet(apps_init_tid, 0x2);
}

static void app_poweron_scan(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    pwron_case = APP_POWERON_CASE_BOTHSCAN;

    osSignalSet(apps_init_tid, 0x2);
}

#ifdef __ENGINEER_MODE_SUPPORT__
static void app_poweron_factorymode(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    app_factorymode_enter();
}
#endif

static void app_poweron_finished(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    osSignalSet(apps_init_tid, 0x2);
}

void app_poweron_wait_finished(void)
{
    osSignalWait(0x2, osWaitForever);
}

#if  defined(__POWERKEY_CTRL_ONOFF_ONLY__)
void app_bt_key_shutdown(APP_KEY_STATUS *status, void *param);
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},           "power on: shutdown"     , app_bt_key_shutdown, NULL},
};
#elif defined(__ENGINEER_MODE_SUPPORT__)
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITUP},           "power on: normal"     , app_poweron_normal, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGPRESS},    "power on: both scan"  , app_poweron_scan  , NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGLONGPRESS},"power on: factory mode", app_poweron_factorymode  , NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITFINISHED},     "power on: finished"   , app_poweron_finished  , NULL},
};
#else
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITUP},           "power on: normal"     , app_poweron_normal, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGPRESS},    "power on: both scan"  , app_poweron_scan  , NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITFINISHED},     "power on: finished"   , app_poweron_finished  , NULL},
};
#endif
static void app_poweron_key_init(void)
{
    uint8_t i = 0;
    TRACE("%s",__func__);

    for (i=0; i<(sizeof(pwron_key_handle_cfg)/sizeof(APP_KEY_HANDLE)); i++){
        app_key_handle_registration(&pwron_key_handle_cfg[i]);
    }
}

static uint8_t app_poweron_wait_case(void)
{
	pwron_case = APP_POWERON_CASE_NORMAL;
    return pwron_case;
}

int system_shutdown(void);
int app_shutdown(void)
{
    system_shutdown();
    return 0;
}

int system_reset(void);
int app_reset(void)
{
    system_reset();
    return 0;
}

extern 	"C" enum HAL_KEY_EVENT_T hal_bt_key_read(enum HAL_KEY_CODE_T code);
enum APP_KEY_EVENT_T app_bt_key_read(enum APP_KEY_CODE_T code)
{
	return (enum APP_KEY_EVENT_T)hal_bt_key_read((enum HAL_KEY_CODE_T)code);;
}

void app_nv_clear_bt_scan_mode(void)
{
//disconnect all cons
	if(!btapp_hfp_call_state_active()) {
	    LinkDisconnectDirectly();
	//set no connect able
	    app_bt_accessmode_set(BT_DEFAULT_ACCESS_MODE_2C);
	//force clear nv
		nv_record_sector_clear();

	//set scanble
		enter_scanmode_status(0);
		}
}


void app_bt_key_shutdown(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
    app_reset();
#else
    app_shutdown();
#endif
}


void app_bt_singleclick(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
}

void app_bt_doubleclick(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
}

void app_power_off(APP_KEY_STATUS *status, void *param)
{
    TRACE("app_power_off\n");
}

extern "C" void OS_NotifyEvm(void);
extern bool btapp_hfp_has_connection(void);
uint8_t app_doublekey_count(void);
void app_bt_key(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
#ifdef __FACTORY_MODE_SUPPORT__
    if (app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN && (status->event == APP_KEY_EVENT_DOUBLECLICK)){
        app_factorymode_languageswitch_proc();
    } else
#endif
    if(app_status_indication_get() == APP_STATUS_INDICATION_PAGESCAN&& !btapp_hfp_has_connection()) {
		enter_scanmode_status(1);
	}
	else if(app_doublekey_count() == 0){
        bt_key_send(status->code, status->event);
    }
}

void app_bt_funckey(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);

    if(btapp_hfp_has_connection()&&!btapp_hfp_call_state_active()) {
		LinkDisconnectDirectly();
		enter_scanmode_status(0);
	} else if(app_status_indication_get() == APP_STATUS_INDICATION_PAGESCAN&& !btapp_hfp_has_connection()) {
		enter_scanmode_status(1);
	} else  {
        bt_key_send(status->code, status->event);
    }
}

#if defined(__APP_KEY_FN_STYLE_A__)
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGLONGPRESS},"bt function key",app_bt_key_shutdown, NULL},
	{{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
	{{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS3S},"bt function key",app_bt_funckey, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_LONGPRESS},"bt play forward key",app_bt_key, NULL},
#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif
};
#else //#elif defined(__APP_KEY_FN_STYLE_B__)
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGLONGPRESS},"bt function key",app_bt_key_shutdown, NULL},
	{{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS3S},"bt function key",app_bt_funckey, NULL},
	{{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_REPEAT},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_REPEAT},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt play forward key",app_bt_key, NULL},
#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif
};
#endif

void app_key_init(void)
{
    uint8_t i = 0;
    TRACE("%s",__func__);
    for (i=0; i<(sizeof(app_key_handle_cfg)/sizeof(APP_KEY_HANDLE)); i++){
        app_key_handle_registration(&app_key_handle_cfg[i]);
    }
}

#ifdef __AUTOPOWEROFF__

void CloseEarphone(void)
{
    int activeCons;
    OS_LockStack();
    activeCons = MEC(activeCons);
    OS_UnlockStack();

    if(activeCons == 0)
    {
      TRACE("!!!CloseEarphone\n");
      app_shutdown();
    }
}
#endif /* __AUTOPOWEROFF__ */


void a2dp_suspend_music_force(void);

int app_deinit(int deinit_case)
{
    int nRet = 0;
    TRACE("%s case:%d",__func__, deinit_case);

    if (!deinit_case){
#if FPGA==0
    nv_record_flash_flush();
#endif
    app_poweroff_flag = 1;
    app_status_indication_filter_set(APP_STATUS_INDICATION_BOTHSCAN);
    app_bt_stream_volumeset(TGT_VOLUME_LEVE_0);
    a2dp_suspend_music_force();
    osDelay(200);
    LinkDisconnectDirectly();
    osDelay(500);
    app_status_indication_set(APP_STATUS_INDICATION_POWEROFF);
    app_voice_report(APP_STATUS_INDICATION_POWEROFF, 0);
    osDelay(2000);
#ifdef __CODEC_ASYNC_CLOSE__
    codec_best1000_real_close();
#else
    codec_best1000_close();
#endif
    }

    return nRet;
}

#ifdef APP_TEST_MODE
extern void app_test_init(void);
int app_init(void)
{
    int nRet = 0;
    uint8_t pwron_case = APP_POWERON_CASE_INVALID;
    TRACE("%s",__func__);
    app_poweroff_flag = 0;

    apps_init_tid = osThreadGetId();
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);
    af_open();
    app_os_init();
    app_pwl_open();
    app_audio_open();
    app_audio_manager_open();
    app_overlay_open();
    if (app_key_open(true))
    {
        nRet = -1;
        goto exit;
    }

    app_test_init();
exit:
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    return nRet;
}
#else

#define NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS (0xffffaa55)
int app_bt_connect2tester_init(void)
{
    BtDeviceRecord rec;
    BT_BD_ADDR tester_addr;
	uint8_t i;
	bool find_tester = false;
	struct nvrecord_env_t *nvrecord_env;
	nv_record_env_get(&nvrecord_env);

	if (nvrecord_env->factory_tester_status.status != NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT)
		return 0;

	if (!nvrec_dev_get_dongleaddr(&tester_addr)){
		nv_record_open(section_usrdata_ddbrecord);
		for (i = 0; nv_record_enum_dev_records(i, &rec) == BT_STATUS_SUCCESS; i++) {
			if (!memcmp(rec.bdAddr.addr, tester_addr.addr, BD_ADDR_SIZE)){
				find_tester = false;
			}
		}
		if(i==0){
			nv_record_add(section_usrdata_ddbrecord, &tester_addr);
		}
		if (find_tester && i>2){
			nv_record_ddbrec_delete(&tester_addr);
			nvrecord_env->factory_tester_status.status = NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS;
			nv_record_env_set(nvrecord_env);
		}
	}

	return 0;
}

int app_nvrecord_rebuild(void)
{
	struct nvrecord_env_t *nvrecord_env;
	nv_record_env_get(&nvrecord_env);

	nv_record_sector_clear();
	nv_record_open(section_usrdata_ddbrecord);
	nvrecord_env->factory_tester_status.status = NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS;
	nv_record_env_set(nvrecord_env);
	nv_record_flash_flush();

	return 0;
}

#if defined(USB_AUDIO_MODE)
#include "app_audio.h"
#include "usb_audio_frm_defs.h"
#include "usb_audio_test.h"

void app_usb_key(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
	app_uaudio_mailbox_put(0,(uint32_t)status);

}

const APP_KEY_HANDLE  app_usb_handle_cfg[] = {
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOWN},"USB HID FN1 DOWN key",app_usb_key, NULL},
	{{APP_KEY_CODE_FN1,APP_KEY_EVENT_CLICK},"USB HID FN1 CLICK key",app_usb_key, NULL},
	{{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"USB HID FN1 UP key",app_usb_key, NULL},
	{{APP_KEY_CODE_FN2,APP_KEY_EVENT_DOWN},"USB HID FN2 DOWN key",app_usb_key, NULL},
	{{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"USB HID FN2 UP key",app_usb_key, NULL},
	{{APP_KEY_CODE_FN2,APP_KEY_EVENT_CLICK},"USB HID FN2 CLICK key",app_usb_key, NULL},
};

void app_usb_key_init(void)
{
    uint8_t i = 0;
    TRACE("%s",__func__);
    for (i=0; i<(sizeof(app_usb_handle_cfg)/sizeof(APP_KEY_HANDLE)); i++){
        app_key_handle_registration(&app_usb_handle_cfg[i]);
    }
}


#endif

extern 	"C" void uart_bridge_ble_test(void);
extern 	"C" int app_uaudio_init(void);

extern uint32_t __overlay_bt_player_start;
extern uint32_t __overlay_bt_player_end;
extern uint32_t __overlay_local_player_start;
extern uint32_t __overlay_local_player_end;
static int init_player(bool usb)
{
    uint32_t *start, *end;

    if (!usb) {

        start = &__overlay_bt_player_start;
        end = &__overlay_bt_player_end;
    } else {
        start = &__overlay_local_player_start;
        end = &__overlay_local_player_end;
    }

    while (start < end) {
        *start++ = 0;
    }


    return 0;
}

int app_init(void)
{
    int nRet = 0;
	struct nvrecord_env_t *nvrecord_env;

#ifdef MEDIA_PLAYER_SUPPORT
    UINT32 nLan;
#endif
    uint8_t pwron_case = APP_POWERON_CASE_INVALID;
    TRACE("app_init\n");
#ifdef __WATCHER_DOG_RESET__
    app_wdt_open(15);
#endif

    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);
    apps_init_tid = osThreadGetId();
    app_os_init();
    app_status_indication_init();
    if (app_battery_open()){
		init_player(true);
        pwron_case = APP_POWERON_CASE_CHARGING;
        app_status_indication_set(APP_STATUS_INDICATION_CHARGING);
        app_battery_start();
        btdrv_start_bt();
        btdrv_hciopen();
        btdrv_sleep_config(1);
        btdrv_hcioff();
		app_key_open(true);
        goto exit;
    }
	init_player(false);

    if (app_key_open(true)){
        nRet = -1;
        goto exit;
    }
    app_bt_init();
    af_open();
    app_audio_open();
    app_audio_manager_open();
    app_overlay_open();

    nv_record_env_init();
    nvrec_dev_data_open();
	app_bt_connect2tester_init();
	nv_record_env_get(&nvrecord_env);
    app_play_audio_set_lang(nvrecord_env->media_language.language);
//    a2dp_volume_local_set(nvrecord_env->stream_volume.a2dp_vol);
//    hfp_volume_local_set(nvrecord_env->stream_volume.hfp_vol);
    app_poweron_key_init();
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
	app_status_indication_set(APP_STATUS_INDICATION_INITIAL);
#endif
    app_status_indication_set(APP_STATUS_INDICATION_POWERON);
    app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
    pwron_case = app_poweron_wait_case();
	

#ifdef __ENGINEER_MODE_SUPPORT__
    if (pwron_case == APP_POWERON_CASE_TEST){
        TRACE("!!!!!ENGINEER_MODE!!!!!\n");
        nRet = 0;
        btdrv_start_bt();
        app_poweron_wait_finished();
        app_nvrecord_rebuild();
        app_factorymode_key_init();
    }else
#endif
    if (pwron_case != APP_POWERON_CASE_INVALID && pwron_case != APP_POWERON_CASE_DITHERING){
        TRACE("hello world i'm bes1000 hahaha case:%d\n", pwron_case);
        nRet = 0;
        btdrv_start_bt();
        IabtInit();
        osDelay(1000);
        TRACE("\n\n\nbt_stack_init_done:%d\n\n\n", pwron_case);
        switch (pwron_case) {
            case APP_POWERON_CASE_CALIB:
                break;
            case APP_POWERON_CASE_BOTHSCAN:
            case APP_POWERON_CASE_NORMAL:
            case APP_POWERON_CASE_REBOOT:
            case APP_POWERON_CASE_ALARM:
            default:
             // app_status_indication_set(APP_STATUS_INDICATION_PAGESCAN);

                app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
                app_voice_report(APP_STATUS_INDICATION_BOTHSCAN,0);
#ifdef __EARPHONE__
                app_bt_accessmode_set(BT_DEFAULT_ACCESS_MODE_PAIR);
                app_start_10_second_timer(APP_PAIR_TIMER_ID);
#endif
                break;
        }
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
        app_poweron_wait_finished();
#endif
#if defined( __EARPHONE__) && defined(__BT_RECONNECT__)
		app_bt_opening_reconnect();
#endif
        app_key_init();
        app_battery_start();
#if defined(__EARPHONE__) && defined(__AUTOPOWEROFF__)
        app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif
    }else{
        af_close();
        app_key_close();
        nRet = -1;
    }

exit:
#if defined(USB_AUDIO_MODE)
	if(pwron_case == APP_POWERON_CASE_CHARGING) {

#ifdef __WATCHER_DOG_RESET__
		app_wdt_close();
#endif
		af_open();
		app_audio_open();
		app_audio_manager_open();
		app_overlay_open();
		app_uaudio_init();
		app_usb_key_init();

	}
#endif
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    return nRet;
}

void restart_app_10s_timers(void) {
	TRACE("%s \n",__func__);
	app_start_10_second_timer(APP_PAIR_TIMER_ID);
	app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
}
void enter_scanmode_status(int voice) {
	TRACE("%s \n",__func__);
	app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
	if(voice)
		app_voice_report(APP_STATUS_INDICATION_BOTHSCAN,0);
	app_bt_accessmode_set(BT_DEFAULT_ACCESS_MODE_PAIR);
	restart_app_10s_timers();

}

void report_bat_low_status(void)
{
	app_voice_report(APP_STATUS_INDICATION_BAT_PWRON_LOW,0);
}

void app_bat_low_timer_start(void)
{
	app_start_10_second_timer(APP_BATLOW_TIMER_ID);

}

void app_bat_low_timer_stop(void)
{
	app_stop_10_second_timer(APP_BATLOW_TIMER_ID);

}

bool app_bat_low_timer_running(void)
{
    return app_10_second_timer_pending(APP_BATLOW_TIMER_ID);
}

#endif
