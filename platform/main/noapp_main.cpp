#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_dma.h"
#include "hal_trace.h"
#include "hal_iomux.h"
#include "hal_bootmode.h"
#include "cmsis.h"
#include "hwtimer_list.h"
#include "pmu.h"
#ifdef RTOS
#include "cmsis_os.h"
#include "rt_Time.h"
#endif

#ifndef FLASH_FILL
#define FLASH_FILL                      1
#endif

#ifdef RTOS
#define OS_TIME_STR                     "[%2u / %u]"
#define OS_CUR_TIME                     , SysTick->VAL, os_time
#else
#define OS_TIME_STR
#define OS_CUR_TIME
#endif

#if defined(MS_TIME)
#define TIME_STR                        "[%u]" OS_TIME_STR
#define CUR_TIME                        TICKS_TO_MS(hal_sys_timer_get())  OS_CUR_TIME
#elif defined(RAW_TIME)
#define TIME_STR                        "[0x%x]" OS_TIME_STR
#define CUR_TIME                        hal_sys_timer_get()  OS_CUR_TIME
#else
#define TIME_STR                        "[%u / 0x%x]" OS_TIME_STR
#define CUR_TIME                        TICKS_TO_MS(hal_sys_timer_get()), hal_sys_timer_get() OS_CUR_TIME
#endif

static void timer_handler(void *param)
{
    TRACE(TIME_STR " Timer handler: %u", CUR_TIME, (uint32_t)param);
}

const static unsigned char bytes[FLASH_FILL] = { 0x1, };

// GDB can set a breakpoint on the main function only if it is
// declared as below, when linking with STD libraries.
int main(int argc, char *argv[])
{
    HWTIMER_ID timer;
    int ret;

#if (DEBUG_PORT == 2)
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_iic[] = {
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_I2C_SCL, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_I2C_SDA, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_uart1[] = {
        {HAL_IOMUX_PIN_P3_2, HAL_IOMUX_FUNC_UART1_RX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_3, HAL_IOMUX_FUNC_UART1_TX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };
#else
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_uart0[] = {
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_UART0_RX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_UART0_TX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };
#endif

    hal_audma_open();
    hal_gpdma_open();
#if (DEBUG_PORT == 2)
    hal_trace_open(HAL_TRACE_TRANSPORT_UART1);
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_iic, sizeof(pinmux_iic)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_uart1, sizeof(pinmux_uart1)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
#else
    hal_trace_open(HAL_TRACE_TRANSPORT_UART0);
    //hal_iomux_set_uart0_p3_0();
    //hal_iomux_set_uart1_p3_2();
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_uart0, sizeof(pinmux_uart0)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
#endif
    hwtimer_init();
    ret = pmu_open();
    ASSERT(ret == 0, "Failed to open pmu");

    TRACE(TIME_STR " main started: filled@0x%08x", CUR_TIME, (uint32_t)bytes);

    timer = hwtimer_alloc(timer_handler, 0);
    hwtimer_start(timer, MS_TO_TICKS(1000));
    TRACE(TIME_STR " Start timer", CUR_TIME);

    hal_cmu_simu_pass();

#ifdef SIMU
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_FLASH_BOOT);
    hal_cmu_sys_reboot();
#endif

    while (1) {
#ifdef RTOS
        osDelay(4000);
#else
        hal_sys_timer_delay(4000);
#endif
        TRACE(TIME_STR " osDelay done", CUR_TIME);
        hwtimer_start(timer, MS_TO_TICKS(1000));
        TRACE(TIME_STR " Start timer", CUR_TIME);
    }

    while (1);
    return 0;
}

