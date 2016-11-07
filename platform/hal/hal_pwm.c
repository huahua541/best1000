#include "hal_pwm.h"
#include "reg_pwm.h"
#include "hal_trace.h"
#include "cmsis.h"

#define PWM_SLOW_CLOCK                  32000
#define PWM_FAST_CLOCK                  13000000
#define PWM_MAX_VALUE                   0xFFFF

static struct PWM_T * const pwm = (struct PWM_T *)0x40003000;

int hal_pwm_enable(enum HAL_CMU_PWM_ID_T id, const struct HAL_PWM_CFG_T *cfg)
{
    uint32_t mod_freq;
    uint32_t load;
    uint32_t toggle;
    uint32_t lock;
    uint8_t ratio;

    if (id >= HAL_CMU_PWM_ID_QTY) {
        return 1;
    }
    if (cfg->ratio > 100) {
        return 2;
    }

    if (cfg->inv && (cfg->ratio == 0 || cfg->ratio == 100)) {
        ratio = 100 - cfg->ratio;
    } else {
        ratio = cfg->ratio;
    }

    if (ratio == 0) {
        // Output 0 when disabled
        return 0;
    }

    if (ratio == 100) {
        mod_freq = PWM_SLOW_CLOCK;
        load = PWM_MAX_VALUE;
        toggle = PWM_MAX_VALUE;
    } else {
        mod_freq = PWM_SLOW_CLOCK;
        load = mod_freq / cfg->freq;
        toggle = load * ratio / 100;
        if (toggle == 0) {
            toggle = 1;
        }
        // 10% error is allowed
        if (!cfg->sleep_on && ABS((int)(toggle * 100 - load * ratio)) > load * 10) {
            mod_freq = PWM_FAST_CLOCK;
            load = mod_freq / cfg->freq;
            toggle = load * ratio / 100;
        }
        load = PWM_MAX_VALUE + 1 - load;
        toggle = PWM_MAX_VALUE - toggle;
    }

    hal_cmu_clock_enable(HAL_CMU_MOD_O_PWM0 + id);
    hal_cmu_clock_enable(HAL_CMU_MOD_P_PWM);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_PWM0 + id);
    hal_cmu_reset_clear(HAL_CMU_MOD_P_PWM);

    hal_cmu_pwm_set_freq(id, mod_freq);

    lock = int_lock();

    if (id == HAL_CMU_PWM_ID_0) {
        pwm->LOAD01 = SET_BITFIELD(pwm->LOAD01, PWM_LOAD01_0, load);
        pwm->TOGGLE01 = SET_BITFIELD(pwm->TOGGLE01, PWM_TOGGLE01_0, toggle);
    } else if (id == HAL_CMU_PWM_ID_1) {
        pwm->LOAD01 = SET_BITFIELD(pwm->LOAD01, PWM_LOAD01_1, load);
        pwm->TOGGLE01 = SET_BITFIELD(pwm->TOGGLE01, PWM_TOGGLE01_1, toggle);
    } else if (id == HAL_CMU_PWM_ID_2) {
        pwm->LOAD23 = SET_BITFIELD(pwm->LOAD23, PWM_LOAD23_2, load);
        pwm->TOGGLE23 = SET_BITFIELD(pwm->TOGGLE23, PWM_TOGGLE23_2, toggle);
    } else {
        pwm->LOAD23 = SET_BITFIELD(pwm->LOAD23, PWM_LOAD23_3, load);
        pwm->TOGGLE23 = SET_BITFIELD(pwm->TOGGLE23, PWM_TOGGLE23_3, toggle);
    }

    if (cfg->inv) {
        pwm->INV |= (1 << id);
    } else {
        pwm->INV &= ~(1 << id);
    }

    pwm->EN |= (1 << id);

    int_unlock(lock);

    return 0;
}

int hal_pwm_disable(enum HAL_CMU_PWM_ID_T id)
{
    if (id >= HAL_CMU_PWM_ID_QTY) {
        return 1;
    }

    pwm->EN &= ~(1 << id);
    hal_cmu_reset_set(HAL_CMU_MOD_O_PWM0 + id);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_PWM0 + id);
    if (pwm->EN == 0) {
        hal_cmu_reset_set(HAL_CMU_MOD_P_PWM);
        hal_cmu_clock_disable(HAL_CMU_MOD_P_PWM);
    }

    return 0;
}

