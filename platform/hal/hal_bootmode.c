#include "hal_bootmode.h"
#include "hal_cmu_addr.h"
#include "reg_cmu.h"
#include "cmsis.h"

union HAL_HW_BOOTMODE_T hal_hw_bootmode_get(void)
{
    union HAL_HW_BOOTMODE_T bm;

    bm.reg = hal_cmu_get_address()->BOOTMODE & CMU_BOOTMODE_HW_MASK;
    return bm;
}

void hal_hw_bootmode_clear(void)
{
    uint32_t lock;

    lock = int_lock();
    hal_cmu_get_address()->BOOTMODE |= CMU_BOOTMODE_HW_MASK;
    int_unlock(lock);
}

uint32_t hal_sw_bootmode_get(void)
{
    return hal_cmu_get_address()->BOOTMODE & CMU_BOOTMODE_SW_MASK;
}

void hal_sw_bootmode_set(uint32_t bm)
{
    uint32_t lock;

    bm &= CMU_BOOTMODE_SW_MASK;
    lock = int_lock();
    hal_cmu_get_address()->BOOTMODE |= bm;
    int_unlock(lock);
}

void hal_sw_bootmode_clear(uint32_t bm)
{
    uint32_t lock;

    bm = ~bm & CMU_BOOTMODE_SW_MASK;
    lock = int_lock();
    hal_cmu_get_address()->BOOTMODE &= bm;
    int_unlock(lock);
}

