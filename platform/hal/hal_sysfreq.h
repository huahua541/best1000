#ifndef __HAL_SYSFREQ_H__
#define __HAL_SYSFREQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_cmu.h"

enum HAL_SYSFREQ_USER_T {
    HAL_SYSFREQ_USER_I2C,
    HAL_SYSFREQ_USER_SPI,
    HAL_SYSFREQ_USER_UART,
    HAL_SYSFREQ_USER_USB,
    HAL_SYSFREQ_USER_INTERSYS,
    HAL_SYSFREQ_USER_CODEC,
    HAL_SYSFREQ_USER_KEY,
    HAL_SYSFREQ_USER_OVERLAY,
    HAL_SYSFREQ_USER_RESVALID_0,
    HAL_SYSFREQ_USER_RESVALID_1,
    HAL_SYSFREQ_USER_RESVALID_2,
    HAL_SYSFREQ_USER_RESVALID_3,
    
    HAL_SYSFREQ_USER_QTY
};

int hal_sysfreq_req(enum HAL_SYSFREQ_USER_T user, enum HAL_CMU_FREQ_T freq);

int hal_sysfreq_busy(void);

#ifdef __cplusplus
}
#endif

#endif

