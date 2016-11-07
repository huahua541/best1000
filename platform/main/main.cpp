#include "cmsis_os.h"
#include "tgt_hardware.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_dma.h"
#include "hal_iomux.h"
#include "hal_iomuxip.h"
#include "hal_wdt.h"
#include "hal_sleep.h"
#include "hal_bootmode.h"
#include "hal_trace.h"
#include "hal_chipid.h"
#include "hal_cache.h"
#include "cmsis.h"
#include "hwtimer_list.h"
#include "pmu.h"
#include "analog.h"
#include "apps.h"
#include "app_factory.h"
#include "hal_gpio.h"


#define  system_shutdown_wdt_config(seconds) \
                        do{ \
                            hal_wdt_stop(HAL_WDT_ID_0); \
                            hal_wdt_set_irq_callback(HAL_WDT_ID_0, NULL); \
                            hal_wdt_set_timeout(HAL_WDT_ID_0, seconds); \
                            hal_wdt_start(HAL_WDT_ID_0); \
                            hal_sleep_set_lowpower_hook(HAL_SLEEP_HOOK_USER_0, NULL); \
                        }while(0)

#define REG_SET_FIELD(reg, mask, shift, v)\
                        do{ \
                            *(volatile unsigned int *)(reg) &= ~(mask<<shift); \
                            *(volatile unsigned int *)(reg) |= (v<<shift); \
                        }while(0)

static osThreadId main_thread_tid = NULL;

int system_shutdown(void)
{
    osThreadSetPriority(main_thread_tid, osPriorityRealtime);
    osSignalSet(main_thread_tid, 0x4);
    return 0;
}

int system_reset(void)
{
    osThreadSetPriority(main_thread_tid, osPriorityRealtime);
    osSignalSet(main_thread_tid, 0x8);
    return 0;
}

int tgt_hardware_setup(void)
{
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)cfg_hw_pinmux_pwl, sizeof(cfg_hw_pinmux_pwl)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
    return 0;
}

static void BootInit(void)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2){
        hal_cache_writebuffer_enable(HAL_CACHE_ID_D_CACHE, HAL_CACHE_YES);
    }else{
        hal_cache_writebuffer_enable(HAL_CACHE_ID_D_CACHE, HAL_CACHE_NO);
    }
}

int main(int argc, char *argv[])
{
    uint8_t sys_case = 0;
    int ret = 0;
    uint32_t bootmode = hal_sw_bootmode_get();

    tgt_hardware_setup();

    main_thread_tid = osThreadGetId();

    hal_sys_timer_open();
    hwtimer_init();

    hal_dma_set_delay_func((HAL_DMA_DELAY_FUNC)osDelay);
    hal_audma_open();
    hal_gpdma_open();
    

#if defined( DEBUG)
#if (DEBUG_PORT == 0)
        #error "HAL_TRACE_TRANSPORT_USB NOT SUPPORT"
#elif (DEBUG_PORT == 1)
        const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_uart0[] = {
            {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_UART0_RX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
            {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_UART0_TX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        };
#ifdef __FACTORY_MODE_SUPPORT__
        if (!(bootmode & HAL_SW_BOOTMODE_FACTORY))
#endif
        {
            hal_trace_open(HAL_TRACE_TRANSPORT_UART0);
        }
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_uart0, sizeof(pinmux_uart0)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
#elif (DEBUG_PORT == 2)
        const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_uart1[] = {
            {HAL_IOMUX_PIN_P3_2, HAL_IOMUX_FUNC_UART1_RX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
            {HAL_IOMUX_PIN_P3_3, HAL_IOMUX_FUNC_UART1_TX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        };
        const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_iic[] = {
            {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_I2C_SCL, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
            {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_I2C_SDA, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        };
#ifdef __FACTORY_MODE_SUPPORT__
        if (!(bootmode & HAL_SW_BOOTMODE_FACTORY))
#endif
        {
            hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_iic, sizeof(pinmux_iic)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));        
        }
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_uart1, sizeof(pinmux_uart1)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
        hal_trace_open(HAL_TRACE_TRANSPORT_UART1);
#endif
#endif
    
    // disable bt spi access ana interface
    uint32_t v = 0;
    v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX44_REG_OFFSET);
    v &= ~(1<<2); 
    iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX44_REG_OFFSET);


    // Open analogif (ISPI) after setting ISPI module freq
    ret = hal_analogif_open();
    if (ret) {
        hal_cmu_simu_tag(31);
        while (1);
    }
    hal_chipid_init();
    BootInit();
    TRACE("\nchip metal id = %x\n", hal_get_chip_metal_id());

    pmu_open();
    //fix me set digital gain
    REG_SET_FIELD(0x4000a050, 0x07, 24, 4); // [26:24] ch0 adc hbf3_sel 0 for *8 1 for *10 2 for *11 3 for *12 4 for *13 5 for *16
    REG_SET_FIELD(0x4000a050, 0x07, 27, 4); // [29:27] ch1 adc hbf3_sel
    REG_SET_FIELD(0x4000a050, 0x0f,  7, 6); // [10:7]  ch1 adc volume mic gain -12db(0001)~16db(1111) mute(0000)

    REG_SET_FIELD(0x4000a048, 0x0f, 25, 0x06); // // [28:25]  ch0 adc volume mic gain -12db(0001)~16db(1111) mute(0000)
    
    anlog_open();
#if 0
    {
    const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_clkout[] = {HAL_IOMUX_PIN_P2_6, HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE};
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_clkout, sizeof(pinmux_clkout)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
    hal_cmu_clock_out_enable(HAL_CMU_CLOCK_OUT_HCLK);
    }
#endif
    hal_cmu_simu_init();
#ifdef __FACTORY_MODE_SUPPORT__
    if (bootmode & HAL_SW_BOOTMODE_FACTORY){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_FACTORY);
        ret = app_factorymode_init(bootmode);

    }else if(bootmode & HAL_SW_BOOTMODE_CALIB){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_CALIB);
        app_factorymode_calib_only();
    }else
#endif
    {
        ret = app_init();
    }

    if (!ret){
        while(1)
        {
            osEvent evt;
            osSignalClear (main_thread_tid, 0x0f);
            //wait any signal
            evt = osSignalWait(0x0, osWaitForever);

            //get role from signal value
            if(evt.status == osEventSignal)
            {
                if(evt.value.signals & 0x04)
                {
                    sys_case = 1;
                    break;
                }
                else if(evt.value.signals & 0x08)
                {
                    sys_case = 2;
                    break;
                }
            }else{
                sys_case = 1;
                break;
            }
         }
    }
#ifdef __WATCHER_DOG_RESET__
    system_shutdown_wdt_config(10);
#endif
    app_deinit(ret);
    TRACE("byebye~~~ %d\n", sys_case);
    if ((sys_case == 1)||(sys_case == 0)){
        TRACE("shutdown\n");
        pmu_shutdown();
    }else if (sys_case == 2){
        TRACE("reset\n");
        hal_cmu_sys_reboot();
    }
    return 0;
}

