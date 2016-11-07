#include "hal_timer.h"
#include "hal_timer_raw.h"
#include "reg_timer.h"
#include "cmsis_nvic.h"

// For sleep functions
#if defined(ROM_BUILD) || defined(PROGRAMMER)
#define BOOT_TEXT_SRAM_LOC
#define BOOT_DATA_LOC
#else
#define BOOT_TEXT_SRAM_LOC              __attribute__((section(".boot_text_sram")))
#define BOOT_DATA_LOC                   __attribute__((section(".boot_data")))
#endif

static struct DUAL_TIMER_T * const BOOT_DATA_LOC dual_timer = (struct DUAL_TIMER_T *)0x40004000;

static HAL_TIMER_IRQ_HANDLER_T irq_handler = NULL;

//static uint32_t load_value = 0;
static uint32_t start_time;

static void hal_timer1_irq_handler(void);
static void hal_timer2_irq_handler(void);

void hal_sys_timer_open(void)
{
    dual_timer->timer[0].Control = TIMER_CTRL_EN | TIMER_CTRL_PRESCALE_DIV_1 | TIMER_CTRL_SIZE_32_BIT;
    NVIC_SetVector(TIMER1_IRQn, (uint32_t)hal_timer1_irq_handler);
}

uint32_t BOOT_TEXT_SRAM_LOC hal_sys_timer_get(void)
{
    return (~0 - dual_timer->timer[0].Value);
}

uint32_t hal_sys_timer_get_max(void)
{
    return 0xFFFFFFFF;
}

void BOOT_TEXT_SRAM_LOC hal_sys_timer_delay(uint32_t ticks)
{
    uint32_t start = dual_timer->timer[0].Value;

    while (start - dual_timer->timer[0].Value < ticks);
}

static void hal_timer1_irq_handler(void)
{
    dual_timer->timer[0].IntClr = 0;
    dual_timer->timer[0].Control &= ~TIMER_CTRL_INTEN;
}

void hal_timer_setup(enum HAL_TIMER_TYPE_T type, HAL_TIMER_IRQ_HANDLER_T handler)
{
    uint32_t mode;

    if (type == HAL_TIMER_TYPE_ONESHOT) {
        mode = TIMER_CTRL_ONESHOT;
    } else if (type == HAL_TIMER_TYPE_PERIODIC) {
        mode = TIMER_CTRL_MODE_PERIODIC;
    } else {
        mode = 0;
    }

    irq_handler = handler;

    dual_timer->timer[1].IntClr = 0;
    dual_timer->elapsed_timer[1].ElapsedCtrl = TIMER_ELAPSED_CTRL_CLR;

    if (handler) {
        NVIC_SetVector(TIMER2_IRQn, (uint32_t)hal_timer2_irq_handler);
        NVIC_SetPriority(TIMER2_IRQn, IRQ_PRIORITY_NORMAL);
        NVIC_EnableIRQ(TIMER2_IRQn);
    }

    dual_timer->timer[1].Control = mode |
                                   (handler ? TIMER_CTRL_INTEN : 0) |
                                   TIMER_CTRL_PRESCALE_DIV_1 |
                                   TIMER_CTRL_SIZE_32_BIT;
}

void hal_timer_start(uint32_t load)
{
    start_time = hal_sys_timer_get();
    hal_timer_reload(load);
    hal_timer_continue();
}

void hal_timer_stop(void)
{
    dual_timer->timer[1].Control &= ~TIMER_CTRL_EN;
    dual_timer->elapsed_timer[1].ElapsedCtrl = TIMER_ELAPSED_CTRL_CLR;
    dual_timer->timer[1].IntClr = 0;
    NVIC_ClearPendingIRQ(TIMER2_IRQn);
}

void hal_timer_continue(void)
{
    dual_timer->elapsed_timer[1].ElapsedCtrl = TIMER_ELAPSED_CTRL_EN | TIMER_ELAPSED_CTRL_CLR;
    dual_timer->timer[1].Control |= TIMER_CTRL_EN;
}

int hal_timer_is_enabled(void)
{
    return !!(dual_timer->timer[1].Control & TIMER_CTRL_EN);
}

void hal_timer_reload(uint32_t load)
{
    if (load > HAL_TIMER_LOAD_DELTA) {
        //load_value = load;
        load -= HAL_TIMER_LOAD_DELTA;
    } else {
        //load_value = HAL_TIMER_LOAD_DELTA + 1;
        load = 1;
    }
    dual_timer->timer[1].Load = load;
}

uint32_t hal_timer_get(void)
{
    return dual_timer->timer[1].Value;
}

int hal_timer_irq_active(void)
{
    return NVIC_GetActive(TIMER2_IRQn);
}

int hal_timer_irq_pending(void)
{
    // Or NVIC_GetPendingIRQ(TIMER2_IRQn) ?
    return (dual_timer->timer[1].MIS & TIMER_MIS_MIS);
}

uint32_t hal_timer_get_overrun_time(void)
{
    uint32_t extra;

    if (dual_timer->elapsed_timer[1].ElapsedCtrl & TIMER_ELAPSED_CTRL_EN) {
        extra = dual_timer->elapsed_timer[1].ElapsedVal;
    } else {
        extra = 0;
    }

    return extra;
}

uint32_t hal_timer_get_elapsed_time(void)
{
    //return load_value + hal_timer_get_overrun_time();
    return hal_sys_timer_get() - start_time;
}

static void hal_timer2_irq_handler(void)
{
    uint32_t elapsed;

    dual_timer->timer[1].IntClr = 0;
    if (irq_handler) {
        elapsed = hal_timer_get_elapsed_time();
        irq_handler(elapsed);
    } else {
        dual_timer->timer[1].Control &= ~TIMER_CTRL_INTEN;
    }
}

