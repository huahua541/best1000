#ifndef __BT_DRV_INTERFACE_H__
#define  __BT_DRV_INTERFACE_H__

#include "stdint.h"
#include "stdbool.h"

void btdrv_hciopen(void);
void btdrv_hcioff(void);
void btdrv_start_bt(void);
void btdrv_enter_testmode(void);
void btdrv_enable_dut(void);
void btdrv_enable_autoconnector(void);
void btdrv_hci_reset(void);
void btdrv_enable_nonsig_tx(uint8_t index);
void btdrv_enable_nonsig_rx(uint8_t index);
void bt_drv_calib_open(void);
void bt_drv_calib_close(void);
int bt_drv_calib_result_porc(uint32_t *capval);
void bt_drv_calib_rxonly_porc(void);

#ifdef __cplusplus
extern "C" {
#endif
void btdrv_sleep_config(uint8_t sleep_en);
int btdrv_meinit_param_init(void);
int btdrv_meinit_param_remain_size_get(void);
int btdrv_meinit_param_next_entry_get(uint32_t *addr, uint32_t *val);

void btdrv_store_device_role(bool slave);
bool btdrv_device_role_is_slave(void);

void btdrv_rf_init_xtal_fcap(uint32_t fcap);
uint32_t btdrv_rf_get_xtal_fcap(void);
void btdrv_rf_set_xtal_fcap(uint32_t fcap);
int btdrv_rf_xtal_fcap_busy(void);

void btdrv_rf_bit_offset_track_enable(bool enable);
uint32_t btdrv_rf_bit_offset_get(void);

#ifdef __cplusplus
}
#endif

#endif

