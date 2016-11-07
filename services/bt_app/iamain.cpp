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
#include "apps.h"
#include "app_status_ind.h"
#include "app_utils.h"
#include "app_bt_stream.h"
#include "nvrecord_dev.h"
#include "tgt_hardware.h"
#include "iabt_cfg.h"

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
#include "hid.h"

#if IAG_BLE_INCLUDE == XA_ENABLED
extern void bridge_ble_stack_init(void);
extern void bridge_process_ble_event(void);
extern void bridge_gatt_client_proc_thread();
#endif
void IAHCI_Open(void);
void IAHCI_Poll(void);
void IAHCI_SCO_Data_Start(void);
void IAHCI_SCO_Data_Stop(void);
void IAHCI_LockBuffer(void);
void IAHCI_UNLockBuffer(void);
}
#include "rtos.h"
#include "iabt.h"

#include "cqueue.h"
#include "btapp.h"
#include "app_bt.h"

struct iabt_cfg_t iabt_cfg = {
#ifdef __BT_SNIFF__
    .sniff = true,
#else
    .sniff = false,
#endif
#ifdef __BT_ONE_BRING_TWO__
    .one_bring_two = true,
#else
    .one_bring_two = false,
#endif
};

osMessageQDef(evm_queue, 64, message_t);
osMessageQId  evm_queue_id;

/* iabt thread */
#if IAG_BLE_INCLUDE == XA_ENABLED
#define IABT_STACK_SIZE 1024*5
#else
#define IABT_STACK_SIZE 1024*2
#endif

static osThreadId iabt_tid;
uint32_t os_thread_def_stack_Iabt[IABT_STACK_SIZE / sizeof(uint32_t)];
osThreadDef_t os_thread_def_IabtThread = { (IabtThread), (osPriorityAboveNormal), (IABT_STACK_SIZE), (os_thread_def_stack_Iabt)};



unsigned char BT_INQ_EXT_RSP[INQ_EXT_RSP_LEN];

void iany_refresh_device_name(void)
{
    ME_SetLocalDeviceName((const unsigned char*)BT_LOCAL_NAME, strlen(BT_LOCAL_NAME) + 1);
}


extern struct BT_DEVICE_T  app_bt_device;

extern void a2dp_init(void);
extern void app_hfp_init(void);


unsigned char *randaddrgen_get_bt_addr(void)
{
    return bt_addr;
}

unsigned char *randaddrgen_get_ble_addr(void)
{
    return ble_addr;
}

const char *randaddrgen_get_btd_localname(void)
{
    return BT_LOCAL_NAME;
}

#if HID_DEVICE == XA_ENABLED

void hid_callback(HidChannel *Chan, HidCallbackParms *Info)
{
    TRACE("hid_callback Info->event=%x\n",Info->event);
}

#endif


uint8_t sco_num=0;
int iamain(void)
{
#ifdef RANDOM_BTADDR_GEN
    dev_addr_name devinfo;
#endif

    OS_Init();

    IAHCI_Open();

    for(uint8_t i=0;i<BT_DEVICE_NUM;i++)
    {
    memset(&(app_bt_device.a2dp_stream[i]), 0, sizeof(app_bt_device.a2dp_stream[i]));
    memset(&(app_bt_device.avrcp_channel[i]), 0, sizeof(app_bt_device.avrcp_channel[i]));
    memset(&(app_bt_device.hf_command[i]), 0, sizeof(app_bt_device.hf_command[i]));
    memset(&(app_bt_device.hf_channel[i]), 0, sizeof(app_bt_device.hf_channel[i]));

    }
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
    {
        sco_num = 2;
    }
    else
    {
        sco_num = 1;
    }
#if IAG_BLE_INCLUDE == XA_ENABLED
    bridge_ble_stack_init();
#endif
    /* ia stack init */
    EVM_Init();


    a2dp_init();
    app_hfp_init();
    /* pair callback init */
    pair_handler.callback = pair_handler_func;
    SEC_RegisterPairingHandler(&pair_handler);
#if BT_SECURITY == XA_ENABLED
    auth_handler.callback = auth_handler_func;
    SEC_RegisterAuthorizeHandler(&auth_handler);
#endif

    /* a2dp register */
    a2dp_avdtpcodec.codecType = AVDTP_CODEC_TYPE_SBC;
    a2dp_avdtpcodec.discoverable = 1;
    a2dp_avdtpcodec.elements = (U8 *)&a2dp_codec_elements;
    a2dp_avdtpcodec.elemLen  = 4;

    for(uint8_t i=0;i<BT_DEVICE_NUM;i++)
    {
        app_bt_device.a2dp_stream[i].type = A2DP_STREAM_TYPE_SINK;
        A2DP_Register(&app_bt_device.a2dp_stream[i], &a2dp_avdtpcodec, a2dp_callback);

        /* avrcp register */
        AVRCP_Register(&app_bt_device.avrcp_channel[i], avrcp_callback, AVRCP_CT_CATEGORY_1 | AVRCP_TG_CATEGORY_2);

        /* hfp register */
        HF_Register(&app_bt_device.hf_channel[i], hfp_callback);
#if HID_DEVICE == XA_ENABLED
        Hid_Register(&app_bt_device.hid_channel[i],hid_callback);
#endif
    }

#ifdef __BT_ONE_BRING_TWO__
    HF_SetMasterRole(&app_bt_device.hf_channel[BT_DEVICE_ID_1], TRUE);
#endif

    /* bt local name */
#ifdef RANDOM_BTADDR_GEN
    devinfo.btd_addr = randaddrgen_get_bt_addr();
    devinfo.ble_addr = randaddrgen_get_ble_addr();
    devinfo.localname = randaddrgen_get_btd_localname();
    nvrec_dev_localname_addr_init(&devinfo);
    ME_SetLocalDeviceName((const unsigned char*)devinfo.localname, strlen(devinfo.localname) + 1);
#else
    ME_SetLocalDeviceName((const unsigned char*)BT_LOCAL_NAME, strlen(BT_LOCAL_NAME) + 1);
#endif

#if BT_BEST_SYNC_CONFIG == XA_ENABLED
    /* sync config */
    ME_SetSyncConfig(SYNC_CONFIG_PATH, SYNC_CONFIG_MAX_BUFFER, SYNC_CONFIG_CVSD_BYPASS);
#endif

    /* err report enable */
    CMGR_ScoSetEDR(EDR_ENABLED);

#if HID_DEVICE == XA_ENABLED
//            ME_SetClassOfDevice(COD_MAJOR_PERIPHERAL|COD_MINOR_PERIPH_KEYBOARD);
            ME_SetClassOfDevice(COD_MAJOR_AUDIO|COD_MAJOR_PERIPHERAL|COD_MINOR_AUDIO_HEADSET|COD_MINOR_PERIPH_KEYBOARD |COD_AUDIO|COD_RENDERING);
#else
            ME_SetClassOfDevice(COD_MAJOR_AUDIO|COD_MINOR_AUDIO_HEADSET|COD_AUDIO|COD_RENDERING);
#endif
#if (FPGA ==1) || defined(__EARPHONE_STAY_BOTH_SCAN__)
    MEC(accModeNCON) = BT_DEFAULT_ACCESS_MODE_PAIR;
#else
    MEC(accModeNCON) = BT_DEFAULT_ACCESS_MODE_NCON;
#endif

    app_bt_golbal_handle_init();

    /* ext inquiry response */
    ME_AutoCreateExtInquiryRsp(BT_INQ_EXT_RSP, INQ_EXT_RSP_LEN);
    ME_SetExtInquiryRsp(1,BT_INQ_EXT_RSP, INQ_EXT_RSP_LEN);
#if defined( __EARPHONE__)
#if (FPGA==1) || defined(APP_TEST_MODE)
    app_bt_accessmode_set(BT_DEFAULT_ACCESS_MODE_PAIR);
#endif
#endif
    SEC_SetIoCapabilities(IO_NO_IO);
    ///init bt key
    bt_key_init();

    OS_NotifyEvm();
    while(1)
    {
#ifdef __WATCHER_DOG_RESET__
        app_wdt_ping();
#endif
    app_sysfreq_req(APP_SYSFREQ_USER_APP_1, APP_SYSFREQ_32K);
    osMessageGet(evm_queue_id, osWaitForever);
    app_sysfreq_req(APP_SYSFREQ_USER_APP_1, APP_SYSFREQ_52M);
//    IAHCI_LockBuffer();
#ifdef __LOCK_AUDIO_THREAD__
    bool stream_a2dp_sbc_isrun = app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC);
        if (stream_a2dp_sbc_isrun)
        {
            af_lock_thread();
        }
#endif
    EVM_Process();
    //avrcp_handleKey();
    bt_key_handle();

#if IAG_BLE_INCLUDE == XA_ENABLED
        bridge_process_ble_event();
#endif

#ifdef __LOCK_AUDIO_THREAD__
        if (stream_a2dp_sbc_isrun)
        {
            af_unlock_thread();
        }
#endif
//    IAHCI_UNLockBuffer();
    IAHCI_Poll();
    }

    return 0;
}

void IabtThread(void const *argument)
{
    iamain();
}

void IabtInit(void)
{
    evm_queue_id = osMessageCreate(osMessageQ(evm_queue), NULL);
    /* bt */
    iabt_tid = osThreadCreate(osThread(IabtThread), NULL);
    TRACE("IabtThread: %x\n", iabt_tid);
}



