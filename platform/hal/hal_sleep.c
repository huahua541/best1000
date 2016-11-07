#include "reg_cmu.h"
#include "hal_sleep.h"
#include "hal_location.h"
#include "hal_trace.h"
#include "hal_psram.h"
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "hal_dma.h"
#include "hal_norflash.h"
#include "hal_gpadc.h"
#include "cmsis_nvic.h"

#define HAL_SLEEP_26M_READY_TIMEOUT     MS_TO_TICKS(3)
#define HAL_SLEEP_PLL_READY_TIMEOUT     MS_TO_TICKS(1)

//static uint8_t SRAM_STACK_LOC sleep_stack[128];
static struct CMU_T * const SRAM_DATA_LOC cmu = (struct CMU_T *)0x40000000;

static HAL_SLEEP_HOOK_HANDLER sleep_hook_handler[HAL_SLEEP_HOOK_USER_QTY] = {0};

static bool cpu_sleep_disabled = false;

static uint32_t unsleep_lock = 0;


static int hal_sleep_lpu_busy(void)
{
    if ((cmu->WAKEUP_CLK_CFG & CMU_LPU_AUTO_SWITCH26M) &&
        (cmu->WAKEUP_CLK_CFG & CMU_LPU_26M_READY) == 0) {
        return 1;
    }
    if ((cmu->WAKEUP_CLK_CFG & CMU_LPU_AUTO_SWITCHPLL) &&
        (cmu->WAKEUP_CLK_CFG & CMU_LPU_PLL_READY) == 0) {
        return 1;
    }
    return 0;
}

static int hal_sleep_cpu_busy(void)
{
    if (unsleep_lock || cpu_sleep_disabled || hal_trace_busy() || hal_sleep_lpu_busy() || hal_dma_busy()) {
        return 1;
    } else {
        return 0;
    }
}

int hal_sleep_config_clock(enum HAL_SLEEP_LPU_CLK_CFG_T cfg, uint32_t timer_26m, uint32_t timer_pll)
{
    uint32_t lpu_clk;

    if (cfg >= HAL_SLEEP_LPU_CLK_QTY) {
        return 1;
    }
    if ((timer_26m & (CMU_TIMER_WT26M_MASK >> CMU_TIMER_WT26M_SHIFT)) != timer_26m) {
        return 2;
    }
    if ((timer_pll & (CMU_TIMER_WTPLL_MASK >> CMU_TIMER_WTPLL_SHIFT)) != timer_pll) {
        return 3;
    }
    if (hal_sleep_lpu_busy()) {
        return -1;
    }

    if (cfg == HAL_SLEEP_LPU_CLK_26M) {
        lpu_clk = CMU_LPU_AUTO_SWITCH26M;
    } else if (cfg == HAL_SLEEP_LPU_CLK_PLL) {
        lpu_clk = CMU_LPU_AUTO_SWITCHPLL | CMU_LPU_AUTO_SWITCH26M;
    } else {
        lpu_clk = 0;
    }
    cmu->WAKEUP_CLK_CFG = CMU_TIMER_WT26M(timer_26m) | CMU_TIMER_WTPLL(0) | lpu_clk;
    if (timer_pll) {
        hal_sys_timer_delay(2);
        cmu->WAKEUP_CLK_CFG = CMU_TIMER_WT26M(timer_26m) | CMU_TIMER_WTPLL(timer_pll) | lpu_clk;
    }
    return 0;
}

int hal_sleep_set_lowpower_hook(enum HAL_SLEEP_HOOK_USER_T user, HAL_SLEEP_HOOK_HANDLER handler)
{
    sleep_hook_handler[user] = handler;
    return 0;
}

static void hal_sleep_lowpower_hook(void)
{
    uint8_t i;
    for (i=0; i<HAL_SLEEP_HOOK_USER_QTY; i++){
        if (sleep_hook_handler[i])
            sleep_hook_handler[i]();
    }
}

static int SRAM_TEXT_LOC NOINLINE hal_sleep_lowpower_mode(void)
{
    int ret;
    int i;
    uint32_t lock;
    uint32_t start;
    uint32_t saved_clk_cfg;
    uint32_t wakeup_cfg;
    uint32_t pll_locked;
    bool irq_pending = false;

    ret = 1;

    lock = int_lock();

    // Check the sleep conditions in interrupt-locked context
    if (hal_sleep_cpu_busy()) {
        // Cannot sleep
    } else if (hal_sysfreq_busy()) {
        // Light sleep
        SCB->SCR = 0;
        __WFI();
    } else {
        // Deep sleep
        hal_sleep_lowpower_hook();

        // Modules (except for psram and flash) sleep
        //psram_sleep();
//        hal_norflash_setup(HAL_NORFLASH_ID_0, DRV_NORFLASH_OP_MODE_QUAD | DRV_NORFLASH_OP_MODE_CONTINUOUS_READ);
        //hal_norflash_sleep(HAL_NORFLASH_ID_0);
        hal_norflash_pre_operation(HAL_NORFLASH_ID_0);

        // Now neither psram nor flash are usable
	//	hal_norflash_sleep(HAL_NORFLASH_ID_0);

        for (i = 0; i < (NVIC_NUM_VECTORS - NVIC_USER_IRQ_OFFSET + 31) / 32; i++) {
            if (NVIC->ICPR[i] & NVIC->ISER[i]) {
                irq_pending = true;
                break;
            }
        }

        if (!irq_pending) {
            ret = 0;
            saved_clk_cfg = cmu->SYS_CLK;
            wakeup_cfg = cmu->WAKEUP_CLK_CFG;

            // Setup wakeup mask
            cmu->WAKEUP_MASK0 = NVIC->ISER[0];
            cmu->WAKEUP_MASK1 = NVIC->ISER[1];

            if (wakeup_cfg & CMU_LPU_AUTO_SWITCHPLL) {
                // Do nothing
                // Hardware will switch system/memory/flash freq to 32K and shutdown PLLs automatically
            } else {
                // Switch memory/flash freq to 26M or 32K
                if (wakeup_cfg & CMU_LPU_AUTO_SWITCH26M) {
                    cmu->SYS_CLK |= CMU_SEL_26M_FLASH;
                    cmu->SYS_CLK |= CMU_SEL_26M_MEM;
                } else {
                    cmu->SYS_CLK |= CMU_SEL_32K_FLASH;
                    cmu->SYS_CLK |= CMU_SEL_32K_MEM;
                }
                // Switch system freq to 26M
                cmu->SYS_CLK |= CMU_SEL_26M_SYS;
                // Shutdown PLLs
                if (saved_clk_cfg & CMU_PU_PLL_AUD) {
                    cmu->SYS_CLK &= ~CMU_EN_PLL_AUD;
                    cmu->SYS_CLK &= ~CMU_PU_PLL_AUD;
                }
                if (saved_clk_cfg & CMU_PU_PLL_USB) {
                    cmu->SYS_CLK &= ~CMU_EN_PLL_USB;
                    cmu->SYS_CLK &= ~CMU_PU_PLL_USB;
                }
                if (wakeup_cfg & CMU_LPU_AUTO_SWITCH26M) {
                    // Do nothing
                    // Hardware will switch system/memory/flash freq to 32K automatically
                } else {
                    // Switch system freq to 32K
                    cmu->SYS_CLK |= CMU_SEL_32K_SYS;
                }
            }

            if (wakeup_cfg & CMU_LPU_AUTO_SWITCH26M) {
                // Enable auto memory retention
                cmu->SLEEP = (cmu->SLEEP & ~CMU_MANUAL_RAM_RETN) |
                    CMU_DEEPSLEEP_EN | CMU_DEEPSLEEP_ROMRAM_EN | CMU_DEEPSLEEP_START;
            } else {
                // Disable auto memory retention
                cmu->SLEEP = (cmu->SLEEP & ~CMU_DEEPSLEEP_ROMRAM_EN) |
                    CMU_DEEPSLEEP_EN | CMU_MANUAL_RAM_RETN | CMU_DEEPSLEEP_START;
            }

            SCB->SCR = SCB_SCR_SLEEPDEEP_Msk;
            __WFI();

			hal_sys_timer_delay(MS_TO_TICKS(1));
            if (wakeup_cfg & CMU_LPU_AUTO_SWITCHPLL) {
                start = hal_sys_timer_get();
                while ((cmu->WAKEUP_CLK_CFG & CMU_LPU_PLL_READY) == 0 &&
                    (hal_sys_timer_get() - start) < HAL_SLEEP_26M_READY_TIMEOUT + HAL_SLEEP_PLL_READY_TIMEOUT);
                // !!! CAUTION !!!
                // Hardware will switch system/memory/flash freq to PLL divider and enable PLLs automatically
            } else {
                // Wait for 26M ready
                if (wakeup_cfg & CMU_LPU_AUTO_SWITCH26M) {
                    start = hal_sys_timer_get();
                    while ((cmu->WAKEUP_CLK_CFG & CMU_LPU_26M_READY) == 0 &&
                        (hal_sys_timer_get() - start) < HAL_SLEEP_26M_READY_TIMEOUT);
                    // Hardware will switch system/memory/flash freq to 26M automatically
                } else {
                    hal_sys_timer_delay(0x20);
                    // Switch system freq to 26M
                    cmu->SYS_CLK &= ~CMU_SEL_32K_SYS;
                }
                // System freq is 26M now and will be restored later
                // Restore PLLs
                if (saved_clk_cfg & (CMU_PU_PLL_AUD | CMU_PU_PLL_USB)) {
                    pll_locked = 0;
                    if (saved_clk_cfg & CMU_PU_PLL_AUD) {
                        cmu->SYS_CLK |= CMU_PU_PLL_AUD;
                        cmu->SYS_CLK |= CMU_EN_PLL_AUD;
                        pll_locked |= CMU_LOCKED_PLL_AUD;
                    }
                    if (saved_clk_cfg & CMU_PU_PLL_USB) {
                        cmu->SYS_CLK |= CMU_PU_PLL_USB;
                        cmu->SYS_CLK |= CMU_EN_PLL_USB;
                        pll_locked |= CMU_LOCKED_PLL_USB;
                    }
                    start = hal_sys_timer_get();
                    while ((cmu->CLK_OUT & pll_locked) != pll_locked &&
                            (hal_sys_timer_get() - start) < HAL_SLEEP_PLL_READY_TIMEOUT);
                }
#if 0
                // Restore memory/flash freq
                if (wakeup_cfg & CMU_LPU_AUTO_SWITCH26M) {
                    cmu->SYS_CLK &= ~CMU_SEL_26M_MEM;
                    cmu->SYS_CLK &= ~CMU_SEL_26M_FLASH;
                } else {
                    cmu->SYS_CLK &= ~CMU_SEL_32K_MEM;
                    cmu->SYS_CLK &= ~CMU_SEL_32K_FLASH;
                }
#endif
            }

            // Restore system/memory/flash freq
            cmu->SYS_CLK = saved_clk_cfg;
        }
//        hal_norflash_setup(HAL_NORFLASH_ID_0, DRV_NORFLASH_OP_MODE_QUAD | DRV_NORFLASH_OP_MODE_CONTINUOUS_READ);
        //hal_norflash_wakeup(HAL_NORFLASH_ID_0);
        hal_norflash_post_operation(HAL_NORFLASH_ID_0);

        //psram_wakeup();

        // Now both psram and flash are usable

 //      hal_gpadc_wakeup();
        // Modules (except for psram and flash) wakeup

        // End of wakeup
    }

    int_unlock(lock);

    return ret;
}

#if 0
static int hal_sleep_lowpower_wrapper(void)
{
    // Assuming MSP is always in internal SRAM
    return hal_sleep_lowpower_mode();
}
#endif

#define RET_int32_t    __r0

#define SVC_ArgN(n) \
  register int __r##n __asm("r"#n);

#define SVC_ArgR(n,t,a) \
  register t   __r##n __asm("r"#n) = a;

#define SVC_Arg0()                                                             \
  SVC_ArgN(0)                                                                  \
  SVC_ArgN(1)                                                                  \
  SVC_ArgN(2)                                                                  \
  SVC_ArgN(3)

#if (defined (__CORTEX_M0)) || defined (__CORTEX_M0PLUS)
#define SVC_Call(f)                                                            \
  __asm volatile                                                                 \
  (                                                                            \
    "ldr r7,="#f"\n\t"                                                         \
    "mov r12,r7\n\t"                                                           \
    "svc 0"                                                                    \
    :               "=r" (__r0), "=r" (__r1), "=r" (__r2), "=r" (__r3)         \
    :                "r" (__r0),  "r" (__r1),  "r" (__r2),  "r" (__r3)         \
    : "r7", "r12", "lr", "cc"                                                  \
  );
#else
#define SVC_Call(f)                                                            \
  __asm volatile                                                                 \
  (                                                                            \
    "ldr r12,="#f"\n\t"                                                        \
    "svc 0"                                                                    \
    :               "=r" (__r0), "=r" (__r1), "=r" (__r2), "=r" (__r3)         \
    :                "r" (__r0),  "r" (__r1),  "r" (__r2),  "r" (__r3)         \
    : "r12", "lr", "cc"                                                        \
  );
#endif

#define SVC_0_1(f,t,rv)                                                           \
__attribute__((always_inline))                                                 \
static inline  t __##f (void) {                                                \
  SVC_Arg0();                                                                  \
  SVC_Call(f);                                                                 \
  return (t) rv;                                                               \
}

SVC_0_1(hal_sleep_lowpower_mode, int, RET_int32_t);

int hal_sleep_enter_sleep(void)
{
    int ret;

    ret = 1;

    if (hal_sleep_cpu_busy()) {
        // Check again later and do not let CPU sleep
    } else {
        // Try sleep
#ifdef DEBUG
        if (__get_IPSR() != 0) {
            ASSERT(false, "Cannot enter sleep in ISR");
            return ret;
        }
#endif
        switch (__get_CONTROL() & 0x03) {
            case 0x00:                                  // Privileged Thread mode & MSP
                ret = hal_sleep_lowpower_mode();
                break;
            default:
            //case 0x01:                                // Unprivileged Thread mode & MSP
            //case 0x02:                                // Privileged Thread mode & PSP
            //case 0x03:                                // Unprivileged Thread mode & PSP
                ret =__hal_sleep_lowpower_mode();
                break;
        }
    }

    return ret;
}

void hal_sleep_enable_cpu_sleep(void)
{
    cpu_sleep_disabled = false;
}

void hal_sleep_disable_cpu_sleep(void)
{
    cpu_sleep_disabled = true;
}



void hal_sleep_clr_unsleep_lock(uint32_t id)
{
    unsleep_lock &= (~id);
}


void hal_sleep_set_unsleep_lock(uint32_t id)
{
    unsleep_lock |= id;
}


