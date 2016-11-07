#ifndef __APP_BT_H__
#define __APP_BT_H__

#ifdef __cplusplus
extern "C" {
#endif

enum APP_BT_REQ_T {
    APP_BT_REQ_ACCESS_MODE_SET,

    APP_BT_REQ_NUM
};

void PairingTransferToConnectable(void);

void app_bt_golbal_handle_init(void);

void app_bt_opening_reconnect(void);

void app_bt_accessmode_set(BtAccessibleMode mode);

void app_bt_send_request(uint32_t message_id, uint32_t param);

void app_bt_init(void);

#ifdef __cplusplus
}
#endif
#endif /* IABT_H */
