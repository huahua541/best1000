#include "plat_types.h"
#include "hal_sysfreq.h"
#include "hal_location.h"
#include "hal_trace.h"
#include "cmsis.h"

static uint32_t BOOT_BSS_LOC sysfreq_bundle[(HAL_SYSFREQ_USER_QTY + 3) / 4];

static uint8_t * const sysfreq_per_user = (uint8_t *)&sysfreq_bundle[0];

static enum HAL_SYSFREQ_USER_T BOOT_BSS_LOC top_user = HAL_SYSFREQ_USER_QTY;

int hal_sysfreq_req(enum HAL_SYSFREQ_USER_T user, enum HAL_CMU_FREQ_T freq)
{
    uint32_t lock;
    enum HAL_CMU_FREQ_T cur_sys_freq;
    int i;

    if (user >= HAL_SYSFREQ_USER_QTY) {
        return 1;
    }
    if (freq >= HAL_CMU_FREQ_QTY) {
        return 2;
    }

    sysfreq_per_user[user] = freq;

    lock = int_lock();

    cur_sys_freq = hal_cmu_sys_get_freq();

    if (freq > cur_sys_freq) {
        hal_cmu_sys_set_freq(freq);
        top_user = user;
    } else if (freq < cur_sys_freq) {
        // Not to update top_user if the requested freq is lower,
        // even when top_user is HAL_SYSFREQ_USER_QTY
        if (top_user == user) {
            freq = sysfreq_per_user[0];
            user = 0;
            for (i = 1; i < HAL_SYSFREQ_USER_QTY; i++) {
                if (freq < sysfreq_per_user[i]) {
                    freq = sysfreq_per_user[i];
                    user = i;
                }
            }
            if (freq != cur_sys_freq) {
                hal_cmu_sys_set_freq(freq > HAL_CMU_FREQ_26M ? freq : HAL_CMU_FREQ_26M);
                top_user = user;
            }
        }
    } else {
        top_user = user;
    }

    int_unlock(lock);

    return 0;
}

int hal_sysfreq_busy(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(sysfreq_bundle); i++) {
        if (sysfreq_bundle[i] != 0) {
            return 1;
        }
    }

    return 0;
}

void hal_sysfreq_print(void)
{
    int i;

    for (i = 0; i < HAL_SYSFREQ_USER_QTY; i++) {
        if (sysfreq_per_user[i] != 0) {
            TRACE("*** SYSFREQ user=%d freq=%d", i, sysfreq_per_user[i]);
        }
    }
    TRACE("*** SYSFREQ top_user=%d", i, top_user);
}

