#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_aud.h"
#include "apps.h"
#include "app_thread.h"
#include "app_status_ind.h"

#include "nvrecord.h"
#include "app_pwl.h"

extern "C" {
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "a2dp.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "hf.h"
#include "sys/mei.h"
}
#include "btalloc.h"
#include "btapp.h"
#include "app_bt.h"
#include "bt_drv_interface.h"

#if defined(__EARPHONE_STAY_BCR_SLAVE__) && defined(__BT_ONE_BRING_TWO__)
#error can not defined at the same time.
#endif
extern void enter_scanmode_status(int voice);

extern struct BT_DEVICE_T  app_bt_device;
U16 bt_accessory_feature_feature = HF_CUSTOM_FEATURE_SUPPORT;

#ifdef __BT_ONE_BRING_TWO__
BtDeviceRecord record2_copy;
uint8_t record2_avalible;
#endif
BtAccessibleMode gBT_DEFAULT_ACCESS_MODE = BAM_NOT_ACCESSIBLE;

extern int app_pwl_stop(enum APP_PWL_ID_T id);
void app_bt_accessmode_set(BtAccessibleMode mode)
{
    const BtAccessModeInfo info = { BT_DEFAULT_INQ_SCAN_INTERVAL,
                                    BT_DEFAULT_INQ_SCAN_WINDOW,
                                    BT_DEFAULT_PAGE_SCAN_INTERVAL,
                                    BT_DEFAULT_PAGE_SCAN_WINDOW };
    OS_LockStack();

    gBT_DEFAULT_ACCESS_MODE = mode;
    ME_SetAccessibleMode(gBT_DEFAULT_ACCESS_MODE, &info);

    OS_UnlockStack();
}

void PairingTransferToConnectable(void)
{
    int activeCons;
    OS_LockStack();
    activeCons = MEC(activeCons);
    OS_UnlockStack();

    if(activeCons == 0){
        TRACE("!!!PairingTransferToConnectable  BAM_CONNECTABLE_ONLY\n");
       //app_bt_accessmode_set(BAM_CONNECTABLE_ONLY);
       app_status_indication_set(APP_STATUS_INDICATION_PAGESCAN);
	   app_pwl_stop(APP_PWL_ID_0);
	   app_pwl_stop(APP_PWL_ID_1);
    }
}

BOOL app_bt_opening_reconnect_onprocessing(void *ptr)
{
    bool nRet;
#ifdef __BT_ONE_BRING_TWO__
    if (app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_1]||app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_2])
        nRet = true;
    else
        nRet =  false;
#else
    if (app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_1])
        nRet = true;
    else
        nRet =  false;
#endif
    return nRet;
}

void app_bt_opening_reconnect(void)
{
    int ret;
    BtDeviceRecord record1;
    BtDeviceRecord record2;

    OS_LockStack();

    ret = nv_record_enum_latest_two_paired_dev(&record1,&record2);
#ifdef __BT_ONE_BRING_TWO__
    TRACE("!!!app_bt_opening_reconnect  latest_two_paired_dev:\n");
    DUMP8("%x ", &record1.bdAddr, 6);
    DUMP8("%x ", &record2.bdAddr, 6);
    if(ret > 0){
        TRACE("!!!start reconnect\n");

        app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_1] = 1;
        if(MEC(pendCons) == 0){
            HF_CreateServiceLink(&app_bt_device.hf_channel[BT_DEVICE_ID_1], &record1.bdAddr);
        }
        if(ret ==2)
        {
        //as 1more request
            //record2_copy = record2;
            record2_avalible = 0;
        }
        else
            record2_avalible = 0;
    }
    else
    {
        TRACE("!!!go to pairing\n");
#ifdef __EARPHONE_STAY_BOTH_SCAN__
       // app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
       // app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
       // app_bt_accessmode_set(BT_DEFAULT_ACCESS_MODE_PAIR);
       // app_start_10_second_timer(APP_PAIR_TIMER_ID);
#endif

    }
#else
    if(ret > 0){
        TRACE("!!!app_bt_opening_reconnect  one device:\n");
        DUMP8("%x ", &record1.bdAddr, 6);
        app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_1] = 1;
        if(MEC(pendCons) == 0){
            HF_CreateServiceLink(&app_bt_device.hf_channel[BT_DEVICE_ID_1], &record1.bdAddr);
        }
    }
    else
    {
        TRACE("!!!go to pairing\n");
#ifdef __EARPHONE_STAY_BOTH_SCAN__
        app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
        app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
        app_bt_accessmode_set(BT_DEFAULT_ACCESS_MODE_PAIR);
        app_start_10_second_timer(APP_PAIR_TIMER_ID);
#endif
    }
#endif /* __BT_ONE_BRING_TWO__ */

    OS_UnlockStack();
}

BtHandler  app_bt_handler;
static void app_bt_golbal_handle(const BtEvent *Event)
{
   // TRACE("app_bt_golbal_handle evt = %d",Event->eType);
#ifdef __EARPHONE_STAY_BCR_SLAVE__
    static uint8_t switchrole_cnt = 0;
#endif
    switch (Event->eType) {
        case BTEVENT_LINK_DISCONNECT:
            TRACE("app_bt_golbal_handle evt = %d",Event->eType);
#ifdef __EARPHONE_STAY_BCR_SLAVE__
            switchrole_cnt = 0;
#endif
#ifdef __EARPHONE_STAY_BOTH_SCAN__
            if(MEC(activeCons) == 0)
            {
#ifdef __BT_ONE_BRING_TWO__
                if(app_bt_device.bt_open_recon_flag[BT_DEVICE_ID_2] == 0)
#endif
                {
                   // app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
                   // app_start_10_second_timer(APP_PAIR_TIMER_ID);
                }
            }
#endif
            break;
        case BTEVENT_ROLE_CHANGE:
            TRACE("app_bt_golbal_handle BTEVENT_ROLE_CHANGE eType:0x%x errCode:0x%x newRole:%d", Event->eType, Event->errCode, Event->p.roleChange.newRole);
            btdrv_store_device_role(Event->p.roleChange.newRole == BCR_SLAVE);
#ifdef __EARPHONE_STAY_BCR_SLAVE__
            switch (Event->p.roleChange.newRole) {
                case BCR_MASTER:
                    switchrole_cnt++;
                    if (switchrole_cnt<=3)
                        ME_SwitchRole(Event->p.remDev);
                    break;
                case BCR_SLAVE:
                    break;
                case BCR_ANY:
                    break;
                case BCR_UNKNOWN:
                    break;
                default:
                    break;
            }
#endif
            break;
        case BTEVENT_LINK_CONNECT_CNF:
            TRACE("app_bt_golbal_handle BTEVENT_LINK_CONNECT_CNF eType:0x%x errCode:0x%x remDev.role:%d", Event->eType, Event->errCode, Event->p.remDev->role);
            btdrv_store_device_role(Event->p.remDev->role == BCR_SLAVE);
#ifdef __EARPHONE_STAY_BCR_SLAVE__
            switchrole_cnt = 0;
            if (Event->p.remDev->role == BCR_MASTER){
                switchrole_cnt++;
                ME_SwitchRole(Event->p.remDev);
            }
#endif
            break;
		case BTEVENT_SCO_CONNECT_IND:
		case BTEVENT_SCO_CONNECT_CNF:
			SetLinkPolicy(Event->p.scoConnect.remDev, 0);
			break;
		case BTEVENT_SCO_DISCONNECT:
#ifdef __BT_ONE_BRING_TWO__
			SetLinkPolicy(Event->p.scoConnect.remDev,  BLP_SNIFF_MODE);
#else			
			SetLinkPolicy(Event->p.scoConnect.remDev,  BLP_MASTER_SLAVE_SWITCH|BLP_SNIFF_MODE);
#endif
			break;

        }
}

void app_bt_golbal_handle_init(void)
{
    BtEventMask mask = BEM_NO_EVENTS;
    ME_InitHandler(&app_bt_handler);
    app_bt_handler.callback = app_bt_golbal_handle;
    ME_RegisterGlobalHandler(&app_bt_handler);
    mask |= BEM_ROLE_CHANGE | BEM_SCO_CONNECT_CNF | BEM_SCO_DISCONNECT | BEM_SCO_CONNECT_IND;
#ifdef __EARPHONE_STAY_BOTH_SCAN__
    mask |= BEM_LINK_DISCONNECT;
#endif
#ifdef __EARPHONE_STAY_BCR_SLAVE__
    mask |= BEM_LINK_CONNECT_CNF;
    ME_SetConnectionRole(BCR_ANY);
#endif
    ME_SetEventMask(&app_bt_handler, mask);
}

void app_bt_send_request(uint32_t message_id, uint32_t param)
{
    APP_MESSAGE_BLOCK msg;

    TRACE("app_bt_send_request: %d\n", message_id);
    msg.mod_id = APP_MODUAL_BT;
    msg.msg_body.message_id = message_id;
    msg.msg_body.message_Param0 = param;
    app_mailbox_put(&msg);
}

static int app_bt_handle_process(APP_MESSAGE_BODY *msg_body)
{
    TRACE("app_bt_handle_process: %d\n", msg_body->message_id);

    switch (msg_body->message_id) {
        case APP_BT_REQ_ACCESS_MODE_SET:
            if (msg_body->message_Param0 == BT_DEFAULT_ACCESS_MODE_PAIR){
				#if FPGA == 0
                //app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
                //app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
				#endif
                //app_bt_accessmode_set(BT_DEFAULT_ACCESS_MODE_PAIR);
				#if FPGA == 0
                //app_start_10_second_timer(APP_PAIR_TIMER_ID);
				#endif
				enter_scanmode_status(0);
            }else if (msg_body->message_Param0 == BAM_CONNECTABLE_ONLY){
                app_bt_accessmode_set(BAM_CONNECTABLE_ONLY);
            }else if (msg_body->message_Param0 == BAM_NOT_ACCESSIBLE){
                app_bt_accessmode_set(BAM_NOT_ACCESSIBLE);
            }
            break;
         default:
            break;
    }

    return 0;
}

void app_bt_init(void)
{
    app_set_threadhandle(APP_MODUAL_BT, app_bt_handle_process);
//    SecSetIoCapRspRejectExt(app_bt_opening_reconnect_onprocessing);
}

