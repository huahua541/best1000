#ifndef __HAL_BOOTMODE_H__
#define __HAL_BOOTMODE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

// SW_BOOTMODE_START                            (1 << 4)
#define HAL_SW_BOOTMODE_READ_ENABLED            (1 << 4)
#define HAL_SW_BOOTMODE_WRITE_ENABLED           (1 << 5)
#define HAL_SW_BOOTMODE_JTAG_ENABLED            (1 << 6)
#define HAL_SW_BOOTMODE_FORCE_USB_DLD           (1 << 7)
#define HAL_SW_BOOTMODE_FORCE_UART_DLD          (1 << 8)
#define HAL_SW_BOOTMODE_DLD_TRANS_UART          (1 << 9)
#define HAL_SW_BOOTMODE_CRASH_DUMP              (1 << 10)
#define HAL_SW_BOOTMODE_CHIP_TEST               (1 << 11)
#define HAL_SW_BOOTMODE_FACTORY                 (1 << 12)
#define HAL_SW_BOOTMODE_CALIB                   (1 << 13)
#define HAL_SW_BOOTMODE_TEST                    (1 << 14)
#define HAL_SW_BOOTMODE_FLASH_BOOT              (1 << 15)
#define HAL_SW_BOOTMODE_REBOOT                  (1 << 16)
#define HAL_SW_BOOTMODE_CRYSTAL_40M             (1 << 17)

// Add new s/w bootmodes here

#define HAL_SW_BOOTMODE_LOCAL_PLAYER            (1 << 31)

// SW_BOOTMODE_END                              (1 << 31)

union HAL_HW_BOOTMODE_T {
    struct {
        uint32_t watchdog : 1;
        uint32_t global   : 1;
        uint32_t rtc      : 1;
        uint32_t charger  : 1;
    };
    uint32_t reg;
};

union HAL_HW_BOOTMODE_T hal_hw_bootmode_get(void);

void hal_hw_bootmode_clear(void);

uint32_t hal_sw_bootmode_get(void);

void hal_sw_bootmode_set(uint32_t bm);

void hal_sw_bootmode_clear(uint32_t bm);

#ifdef __cplusplus
}
#endif

#endif

