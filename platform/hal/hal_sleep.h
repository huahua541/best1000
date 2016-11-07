#ifndef __HAL_SLEEP_H__
#define __HAL_SLEEP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

enum HAL_SLEEP_UNSLEEP_LOCK_T {
    HAL_SLEEP_UNSLEEP_LOCK_INTERSYS =1,
    HAL_SLEEP_UNSLEEP_LOCK_BT=2,
	HAL_SLEEP_UNSLEEP_LOCK_APP=4,
    HAL_SLEEP_UNSLEEP_QTY
};

enum HAL_SLEEP_LPU_CLK_CFG_T {
    HAL_SLEEP_LPU_CLK_NONE,
    HAL_SLEEP_LPU_CLK_26M,
    HAL_SLEEP_LPU_CLK_PLL,

    HAL_SLEEP_LPU_CLK_QTY
};

enum HAL_SLEEP_HOOK_USER_T {
    HAL_SLEEP_HOOK_USER_0 = 0,
    HAL_SLEEP_HOOK_USER_1,
    HAL_SLEEP_HOOK_USER_2,
    HAL_SLEEP_HOOK_USER_3,
    HAL_SLEEP_HOOK_USER_QTY
};

typedef void (*HAL_SLEEP_HOOK_HANDLER)(void);

int hal_sleep_config_clock(enum HAL_SLEEP_LPU_CLK_CFG_T cfg, uint32_t timer_26m, uint32_t timer_pll);

int hal_sleep_enter_sleep(void);

int hal_sleep_set_lowpower_hook(enum HAL_SLEEP_HOOK_USER_T user, HAL_SLEEP_HOOK_HANDLER handler);

void hal_sleep_enable_cpu_sleep(void);

void hal_sleep_disable_cpu_sleep(void);

void hal_sleep_clr_unsleep_lock(uint32_t id);

void hal_sleep_set_unsleep_lock(uint32_t id);

#ifdef __cplusplus
}
#endif

#endif

