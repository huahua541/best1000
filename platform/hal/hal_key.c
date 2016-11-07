#include "stddef.h"
#include "cmsis_nvic.h"
#include "reg_cmu.h"
#include "reg_iomuxip.h"
#include "hal_iomuxip.h"
#include "hal_iomux.h"
#include "hal_timer_raw.h"
#include "hwtimer_list.h"
#include "hal_analogif.h"
#include "hal_sysfreq.h"
#include "hal_trace.h"
#include "hal_chipid.h"
#include "hal_key.h"
#include "hal_location.h"
#include "tgt_hardware.h"

#define HAL_KEY_TRACE(s,...)
// hal_trace_printf(s, ##__VA_ARGS__)

#define key_reg_read(reg,val)  hal_analogif_reg_read(reg,val)
#define key_reg_write(reg,val) hal_analogif_reg_write(reg,val)

//common key define
#define KEY_LONGLONGPRESS_THRESHOLD_MS  (MS_TO_TICKS((2000)))
#define KEY_LONGPRESS_THRESHOLD_3000MS  (MS_TO_TICKS((1000)))
#define KEY_LONGPRESS_THRESHOLD_MS  (MS_TO_TICKS((2000)))
#define KEY_DOUBLECLICK_THRESHOLD_MS (MS_TO_TICKS((500)))
#define KEY_LONGPRESS_REPEAT_THRESHOLD_MS  (MS_TO_TICKS((500)))

//as 1more request
#define KEY_INIT_DOWN_THRESHOLD_MS  (MS_TO_TICKS((3000)))
#define KEY_INIT_LONGPRESS_THRESHOLD_MS  (MS_TO_TICKS((5000)))
#define KEY_INIT_LONGLONGPRESS_THRESHOLD_MS  (MS_TO_TICKS((10000)))

#define KEY_CHECKER_INTERVAL_MS (MS_TO_TICKS((20)))


#define hal_adckey_enable_press_int() do { \
                                            unsigned short read_val; \
                                            /*mask */\
                                            key_reg_read(0x67,&read_val); \
                                            read_val |= (1<<10)|(1<<11)|(1<<12); \
                                            key_reg_write(0x67,read_val); \
                                            /*enable*/ \
                                            key_reg_read(0x68,&read_val); \
                                            read_val |= (1<<10)|(1<<11)|(1<<12); \
                                            key_reg_write(0x68,read_val); \
                                        } while(0)

#define hal_adckey_enable_release_int() do { \
                                            unsigned short read_val; \
                                            /*mask */ \
                                            key_reg_read(0x67,&read_val); \
                                            read_val |= (1<<9)|(1<<11)|(1<<12); \
                                            key_reg_write(0x67,read_val); \
                                            /*enable */  \
                                            key_reg_read(0x68,&read_val); \
                                            read_val |= (1<<9)|(1<<11)|(1<<12); \
                                            key_reg_write(0x68,read_val); \
                                        } while(0)

#define hal_adckey_enable_adc_int() do { \
                                        unsigned short read_val; \
                                        /*mask */ \
                                        key_reg_read(0x67,&read_val); \
                                        read_val |= (1<<7); \
                                        key_reg_write(0x67,read_val); \
                                        /*enable */  \
                                        key_reg_read(0x68,&read_val); \
                                        read_val |= (1<<7); \
                                        key_reg_write(0x68,read_val); \
                                        /*start key adc */ \
                                        key_reg_read(0x65,&read_val); \
                                        read_val |= (1<<9); \
                                        key_reg_write(0x65,read_val); \
                                    } while(0)

#define hal_adckey_disable_allint() do { \
                                        unsigned short read_val; \
                                        /*mask */ \
                                        key_reg_read(0x67,&read_val); \
                                        read_val &= ~((1<<7)|(1<<9)|(1<<10)|(1<<11)|(1<<12)); \
                                        key_reg_write(0x67, read_val); \
                                        /*enable */  \
                                        key_reg_read(0x68,&read_val); \
                                        read_val &= ~((1<<7)|(1<<9)|(1<<10)|(1<<11)|(1<<12)); \
                                        key_reg_write(0x68, read_val); \
                                        /*start key adc */ \
                                        key_reg_read(0x65,&read_val); \
                                        read_val &= ~(1<<9); \
                                        key_reg_write(0x65,read_val); \
                                    } while(0)

#define hal_adckey_reset() do { \
                                key_status.adc_key.volt = 0xffff; \
                                key_status.adc_key.event = HAL_KEY_EVENT_NONE; \
                                key_status.adc_key.time = 0; \
                            } while(0)

#define hal_pwrkey_enable_riseedge_int() do { \
                                            uint32_t v = 0; \
                                            v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET); \
                                            v = IOMUXIP_MUX40_PWRKEY_UP_INT_EN_MASK|IOMUXIP_MUX40_PWRKEY_UP_INT_MASK_MASK; \
                                            iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET); \
                                        } while(0)

#define hal_pwrkey_enable_falledge_int() do { \
                                            uint32_t v = 0; \
                                            v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET); \
                                            v = IOMUXIP_MUX40_PWRKEY_DOWN_INT_EN_MASK|IOMUXIP_MUX40_PWRKEY_DOWN_INT_MASK_MASK; \
                                            iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET); \
                                        } while(0)

#define hal_pwrkey_enable_bothedge_int() do { \
                                            uint32_t v = 0; \
                                            v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET); \
                                            v = IOMUXIP_MUX40_PWRKEY_UP_INT_EN_MASK|IOMUXIP_MUX40_PWRKEY_UP_INT_MASK_MASK| \
                                                IOMUXIP_MUX40_PWRKEY_DOWN_INT_EN_MASK|IOMUXIP_MUX40_PWRKEY_DOWN_INT_MASK_MASK; \
                                            iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET); \
                                        } while(0)

#define hal_pwrkey_disable_int() do { \
                                        uint32_t v = 0; \
                                        v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET);\
                                        v &= ~(IOMUXIP_MUX40_PWRKEY_DOWN_INT_EN_MASK|IOMUXIP_MUX40_PWRKEY_DOWN_INT_MASK_MASK| \
                                        IOMUXIP_MUX40_PWRKEY_UP_INT_EN_MASK|IOMUXIP_MUX40_PWRKEY_UP_INT_MASK_MASK); \
                                        iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET); \
                                    } while(0)

#define hal_pwrkey_reset() do { \
                                key_status.pwr_key.event = HAL_KEY_EVENT_NONE; \
                                key_status.pwr_key.time = 0; \
                            } while(0)

#define hal_gpiokey_enable_allint() do { \
                                        uint8_t i; \
                                        for (i =0; i< CFG_HW_GPIOKEY_NUM; i++){ \
                                            hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)(&(cfg_hw_gpio_key_cfg[i].key_config)), 1); \
                                            hal_gpiokey_setirq((enum HAL_GPIO_PIN_T)(cfg_hw_gpio_key_cfg[i].key_config.pin), HAL_GPIO_IRQ_POLARITY_LOW_FALLING, hal_gpiokey_irqhandler); \
                                        } \
                                    } while(0)

#define hal_gpiokey_disable_allint() do { \
                                        uint8_t i; \
                                        for (i =0; i< CFG_HW_GPIOKEY_NUM; i++){ \
                                            hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)(&(cfg_hw_gpio_key_cfg[i].key_config)), 1); \
                                            hal_gpiokey_setirq((enum HAL_GPIO_PIN_T)(cfg_hw_gpio_key_cfg[i].key_config.pin), HAL_GPIO_IRQ_POLARITY_LOW_FALLING, NULL); \
                                        } \
                                    } while(0)

#define hal_gpiokey_reset() do { \
                                key_status.gpio_key.pin = HAL_GPIO_PIN_NUM; \
                                key_status.gpio_key.event = HAL_KEY_EVENT_NONE; \
                                key_status.gpio_key.time = 0; \
                            } while(0)

#define hal_key_debounce_timer_restart() do { \
                                            if (debounce_timer!= NULL){ \
                                                hwtimer_stop(debounce_timer); \
                                                hwtimer_start(debounce_timer, KEY_CHECKER_INTERVAL_MS); \
                                            } \
                                        } while(0)

static enum HAL_KEY_CODE_T hal_gpiokey_findkey(enum HAL_GPIO_PIN_T pin);
static void hal_gpiokey_setirq(enum HAL_GPIO_PIN_T pin, enum HAL_GPIO_IRQ_POLARITY_T porlariry, void (* HAL_GPIO_PIN_IRQ_HANDLER)(enum HAL_GPIO_PIN_T));
static void hal_gpiokey_irqhandler(enum HAL_GPIO_PIN_T pin);
static enum HAL_KEY_CODE_T hal_adckey_findkey(uint16_t volt);

static int (*key_detected_callback)(uint8_t, uint8_t) = NULL;
static HWTIMER_ID debounce_timer = NULL;

static uint16_t ADCKEY_VOLT_TABLE[CFG_HW_ADCKEY_NUMBER];
static struct HAL_KEY_STATUS_T key_status;

static inline void hal_key_disable_allint(void)
{
#ifndef APP_TEST_MODE
    hal_pwrkey_disable_int();
#endif
    hal_adckey_disable_allint();
    hal_gpiokey_disable_allint();
}

static inline void hal_key_enable_allint(void)
{
#ifndef APP_TEST_MODE
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
    {
        hal_pwrkey_enable_bothedge_int();
    }
    else
    {
#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
        hal_pwrkey_enable_riseedge_int();
#else
        hal_pwrkey_enable_falledge_int();
#endif
     }
#endif
    hal_adckey_enable_press_int();
    hal_gpiokey_enable_allint();
}

static void hal_key_debouncehandler (void)
{
    uint8_t gpio_val;
    uint32_t time;
    enum HAL_KEY_CODE_T key_code = HAL_KEY_CODE_NUM;
    enum HAL_KEY_EVENT_T event = HAL_KEY_EVENT_NONE;
    bool need_timer = false;
    static uint32_t debounce_cnt = 0;
    static uint32_t longkeypress_cnt = 0;
    static bool longlongpress_report = false;
//  HAL_KEY_TRACE("%s\n", __func__);

    if ((debounce_cnt != 0xffffffff) &&
        ((key_status.pwr_key.event!=HAL_KEY_EVENT_NONE)||
        (key_status.adc_key.event!=HAL_KEY_EVENT_NONE)||
        (key_status.gpio_key.event!=HAL_KEY_EVENT_NONE))){
        debounce_cnt++;
    }

    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
    {
        if (key_status.pwr_key.event!=HAL_KEY_EVENT_NONE){
            uint32_t v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET);

            if (key_status.pwr_key.event == HAL_KEY_EVENT_DEBOUNCE){
                need_timer = true;
                if (v & IOMUXIP_MUX40_PWRKEY_VAL_MASK){
                    key_status.pwr_key.event = HAL_KEY_EVENT_DOWN;
                }else {
                    key_status.pwr_key.event = HAL_KEY_EVENT_DITHERING;
                }
            }else if (key_status.pwr_key.event == HAL_KEY_EVENT_DITHERING){
                HAL_KEY_TRACE("key_status.gpio_key DITHERING");
                need_timer = false;
                debounce_cnt = 0;
                hal_pwrkey_reset();
                hal_key_enable_allint();
            }else{
                if (v & IOMUXIP_MUX40_PWRKEY_VAL_MASK){
                    need_timer = true;
                    if (debounce_cnt == 2){
                        HAL_KEY_TRACE("key_status.pwr_key down");
                        key_status.pwr_key.event = HAL_KEY_EVENT_DOWN;
                        key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_DOWN);
                    }else if ((debounce_cnt >= (KEY_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS)) && debounce_cnt!=0xffffffff){
                        HAL_KEY_TRACE("key_status.pwr_key longpress");
                        key_status.pwr_key.event = HAL_KEY_EVENT_LONGPRESS;
                        debounce_cnt = 0xffffffff;
                        key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_LONGPRESS);
                    }
                }else{
                    if (debounce_cnt < (KEY_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS) && (debounce_cnt!=0xffffffff)){
                        key_code = HAL_KEY_CODE_PWR;
                        event = HAL_KEY_EVENT_UP;
                        key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_UP);
                        HAL_KEY_TRACE("key_status.pwr_key up: %d", key_code);
                    }
                    debounce_cnt = 0;
                    need_timer = false;
                    hal_pwrkey_reset();
                    hal_key_enable_allint();
                    HAL_KEY_TRACE("key_status.pwr_key reset");
                }
            }
        }
    }
    else
    {
#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
        if (key_status.pwr_key.event!=HAL_KEY_EVENT_NONE){
            debounce_cnt = 0;
            need_timer = false;
            hal_pwrkey_reset();
            hal_key_enable_allint();
            key_code = HAL_KEY_CODE_PWR;
            event = HAL_KEY_EVENT_UP;
            key_detected_callback(key_code, HAL_KEY_EVENT_UP);
            key_status.pwr_key.event = HAL_KEY_EVENT_NONE;
         }
#else
        if (key_status.pwr_key.event!=HAL_KEY_EVENT_NONE){
            if (key_status.pwr_key.event == HAL_KEY_EVENT_DEBOUNCE){
    //          HAL_KEY_TRACE("key_status.pwr_key DEBOUNCE");
                need_timer = true;
                if (debounce_cnt > 2){
                    HAL_KEY_TRACE("key_status.pwr_key down");
                    key_status.pwr_key.event = HAL_KEY_EVENT_DOWN;
                    key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_DOWN);
                }
            }else if (key_status.pwr_key.event == HAL_KEY_EVENT_DITHERING){
                HAL_KEY_TRACE("key_status.pwr_key DITHERING");
                need_timer = false;
                debounce_cnt = 0;
                hal_pwrkey_reset();
                hal_key_enable_allint();
            }else if (key_status.pwr_key.event == HAL_KEY_EVENT_DOWN){
                if ((debounce_cnt >= (KEY_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS)) && (debounce_cnt!=0xffffffff)){
                    HAL_KEY_TRACE("key_status.pwr_key longpress");
                    key_status.pwr_key.event = HAL_KEY_EVENT_LONGPRESS;
                    debounce_cnt = 0xffffffff;
                    key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_LONGPRESS);
                }
                need_timer = true;
            }else if (key_status.pwr_key.event == HAL_KEY_EVENT_LONGPRESS){
                need_timer = true;
            }else if(key_status.pwr_key.event == HAL_KEY_EVENT_UP){
                if (debounce_cnt < (KEY_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS) && (debounce_cnt!=0xffffffff)){
                    HAL_KEY_TRACE("key_status.pwr_key up");
                    key_code = HAL_KEY_CODE_PWR;
                    event = HAL_KEY_EVENT_UP;
                    key_detected_callback(key_code, HAL_KEY_EVENT_UP);
                }
                debounce_cnt = 0;
                need_timer = false;
                hal_pwrkey_reset();
                hal_key_enable_allint();
    //          HAL_KEY_TRACE("key_status.pwr_key reset");
            }
        }
#endif
    }
    if (key_status.adc_key.event!=HAL_KEY_EVENT_NONE){
        if (key_status.adc_key.event == HAL_KEY_EVENT_DEBOUNCE){
            HAL_KEY_TRACE("key_status.adc_key DEBOUNCE");
            if (debounce_cnt > 2){
                HAL_KEY_TRACE("key_status.adc_key down");
                key_status.adc_key.event = HAL_KEY_EVENT_DOWN;
                key_detected_callback(hal_adckey_findkey(key_status.adc_key.volt), HAL_KEY_EVENT_DOWN);
            }
            hal_adckey_enable_adc_int();
            need_timer = true;
        }else if (key_status.adc_key.event == HAL_KEY_EVENT_DITHERING){
            HAL_KEY_TRACE("key_status.adc_key DITHERING");
            need_timer = false;
            debounce_cnt = 0;
            hal_adckey_reset();
            hal_key_enable_allint();
        }else if (key_status.adc_key.event == HAL_KEY_EVENT_DOWN){
            if ((debounce_cnt >= (KEY_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS)) && (debounce_cnt!=0xffffffff)){
                HAL_KEY_TRACE("key_status.adc_key longpress");
                key_status.adc_key.event = HAL_KEY_EVENT_LONGPRESS;
                debounce_cnt = 0xffffffff;
                key_detected_callback(hal_adckey_findkey(key_status.adc_key.volt), HAL_KEY_EVENT_LONGPRESS);
            }
            hal_adckey_enable_adc_int();
            need_timer = true;
        }else if (key_status.adc_key.event == HAL_KEY_EVENT_LONGPRESS){
            hal_adckey_enable_adc_int();
            need_timer = true;
        }else if(key_status.adc_key.event == HAL_KEY_EVENT_UP){
            if (debounce_cnt < (KEY_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS) && (debounce_cnt!=0xffffffff)){
                key_code = hal_adckey_findkey(key_status.adc_key.volt);
                event = HAL_KEY_EVENT_UP;
                HAL_KEY_TRACE("key_status.adc_key up: %d", key_code);
                key_detected_callback(key_code, HAL_KEY_EVENT_UP);
            }
            debounce_cnt = 0;
            need_timer = false;
            hal_adckey_reset();
            hal_key_enable_allint();
            HAL_KEY_TRACE("key_status.adc_key  reset");
        }
    }

    if (key_status.gpio_key.event!=HAL_KEY_EVENT_NONE){
        gpio_val = hal_gpio_pin_get_val(key_status.gpio_key.pin);
        if (key_status.gpio_key.event == HAL_KEY_EVENT_DEBOUNCE){
            need_timer = true;
            if (gpio_val == CFG_HW_GPIOKEY_DOWN_LEVEL){
                key_status.gpio_key.event = HAL_KEY_EVENT_DOWN;
            }else {
                key_status.gpio_key.event = HAL_KEY_EVENT_DITHERING;
            }
        }else if (key_status.gpio_key.event == HAL_KEY_EVENT_DITHERING){
            HAL_KEY_TRACE("key_status.gpio_key DITHERING");
            need_timer = false;
            debounce_cnt = 0;

            hal_gpiokey_reset();
            hal_key_enable_allint();
        }else{
            if (gpio_val == CFG_HW_GPIOKEY_DOWN_LEVEL){
                need_timer = true;
                if (debounce_cnt == 2){
                    HAL_KEY_TRACE("key_status.gpio_key down: %d", key_status.gpio_key.pin);
                    key_detected_callback(hal_gpiokey_findkey(key_status.gpio_key.pin), HAL_KEY_EVENT_DOWN);
                }else if ((debounce_cnt >= (KEY_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS)) && debounce_cnt!=0xffffffff){
                    HAL_KEY_TRACE("key_status.gpio_key longpress: %d", key_status.gpio_key.pin);
                    key_status.gpio_key.event = HAL_KEY_EVENT_LONGPRESS;
                    debounce_cnt = 0xffffffff;
                    key_detected_callback(hal_gpiokey_findkey(key_status.gpio_key.pin), HAL_KEY_EVENT_LONGPRESS);
                }
            }else if(gpio_val == CFG_HW_GPIOKEY_UP_LEVEL){
                if (debounce_cnt < (KEY_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS) && (debounce_cnt!=0xffffffff)){
                    key_code = hal_gpiokey_findkey(key_status.gpio_key.pin);
                    event = HAL_KEY_EVENT_UP;
                    key_detected_callback(key_code, HAL_KEY_EVENT_UP);
                    HAL_KEY_TRACE("key_status.gpio_key up: %d", key_code);
                }
                debounce_cnt = 0;
                need_timer = false;
                hal_gpiokey_reset();
                hal_key_enable_allint();
                HAL_KEY_TRACE("key_status.gpio_key reset");
            }
        }
    }

    if ((key_status.pwr_key.event == HAL_KEY_EVENT_LONGPRESS) ||
        (key_status.adc_key.event == HAL_KEY_EVENT_LONGPRESS) ||
        (key_status.gpio_key.event == HAL_KEY_EVENT_LONGPRESS) ){

        if (longkeypress_cnt!=0xffffffff){
			longkeypress_cnt++;
        }

        if (!(longkeypress_cnt %(KEY_LONGPRESS_REPEAT_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS))){
			HAL_KEY_TRACE("HAL_KEY_EVENT_REPEAT:%d\n", longkeypress_cnt);
            if (key_status.pwr_key.event == HAL_KEY_EVENT_LONGPRESS)
                key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_REPEAT);

            if (key_status.adc_key.event == HAL_KEY_EVENT_LONGPRESS)
                key_detected_callback(hal_adckey_findkey(key_status.adc_key.volt), HAL_KEY_EVENT_REPEAT);

            if (key_status.gpio_key.event == HAL_KEY_EVENT_LONGPRESS)
                key_detected_callback(hal_gpiokey_findkey(key_status.gpio_key.pin), HAL_KEY_EVENT_REPEAT);
        }

        if ((longkeypress_cnt >= (KEY_LONGLONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS)) && (!longlongpress_report)){
			HAL_KEY_TRACE("HAL_KEY_EVENT_LONGLONGPRESS:%d\n", longkeypress_cnt);
            longlongpress_report = true;
            if (key_status.pwr_key.event == HAL_KEY_EVENT_LONGPRESS)
                key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_LONGLONGPRESS);

            if (key_status.adc_key.event == HAL_KEY_EVENT_LONGPRESS)
                key_detected_callback(hal_adckey_findkey(key_status.adc_key.volt), HAL_KEY_EVENT_LONGLONGPRESS);

            if (key_status.gpio_key.event == HAL_KEY_EVENT_LONGPRESS)
                key_detected_callback(hal_gpiokey_findkey(key_status.gpio_key.pin), HAL_KEY_EVENT_LONGLONGPRESS);
        } else if ((longkeypress_cnt >= (KEY_LONGPRESS_THRESHOLD_3000MS/KEY_CHECKER_INTERVAL_MS)) && (!longlongpress_report)) {
				HAL_KEY_TRACE("HAL_KEY_EVENT_LONGPRESS...3.....S : %d\n", longkeypress_cnt);
	           // longlongpress_report = true;
	            if (key_status.pwr_key.event == HAL_KEY_EVENT_LONGPRESS)
	                key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_LONGPRESS3S);

	            if (key_status.adc_key.event == HAL_KEY_EVENT_LONGPRESS)
	                key_detected_callback(hal_adckey_findkey(key_status.adc_key.volt), HAL_KEY_EVENT_LONGPRESS3S);

	            if (key_status.gpio_key.event == HAL_KEY_EVENT_LONGPRESS)
	                key_detected_callback(hal_gpiokey_findkey(key_status.gpio_key.pin), HAL_KEY_EVENT_LONGPRESS3S);
		}
    }else{
        longkeypress_cnt = 0;
        longlongpress_report = false;
    }

    if ((event == HAL_KEY_EVENT_UP) &&
        (key_code != HAL_KEY_CODE_NUM)&&
        (key_status.key_code == HAL_KEY_CODE_NUM)){
        key_status.key_code = key_code;
        key_status.event = HAL_KEY_EVENT_CLICK;
        key_status.click_time = hal_sys_timer_get();
        need_timer = true;
    }
    if (event == HAL_KEY_EVENT_UP){
        HAL_KEY_TRACE("key_code:%d, key_status.key_code:%d click_time:%d currtime:%d tick:%d",key_code,key_status.key_code, key_status.click_time,hal_sys_timer_get(), KEY_DOUBLECLICK_THRESHOLD_MS);
    }
    if (key_status.event == HAL_KEY_EVENT_CLICK){
        time = hal_sys_timer_get();
        if ((key_status.pwr_key.event!=HAL_KEY_EVENT_NONE)||
            (key_status.adc_key.event!=HAL_KEY_EVENT_NONE)||
            (key_status.gpio_key.event!=HAL_KEY_EVENT_NONE)){
            key_status.event = HAL_KEY_EVENT_DOUBLECLICK;
        }else if ((time-key_status.click_time)>KEY_DOUBLECLICK_THRESHOLD_MS){
            HAL_KEY_TRACE("key_status CLICK:%d", key_status.key_code);
            key_detected_callback(key_status.key_code, HAL_KEY_EVENT_CLICK);
            key_status.key_code = HAL_KEY_CODE_NUM;
            key_status.event = HAL_KEY_EVENT_NONE;
            key_status.click_time = 0;
            need_timer = false;
        }else{
            need_timer = true;
        }
    }else if (key_status.event == HAL_KEY_EVENT_DOUBLECLICK && (event == HAL_KEY_EVENT_UP)){
        time = hal_sys_timer_get();
        if (key_code == key_status.key_code){
            if ((time-key_status.click_time)<KEY_DOUBLECLICK_THRESHOLD_MS){
                HAL_KEY_TRACE("key_status DOUBLECLICK:%d", key_status.key_code);
                key_detected_callback(key_status.key_code, HAL_KEY_EVENT_DOUBLECLICK);
                key_status.key_code = HAL_KEY_CODE_NUM;
                key_status.event = HAL_KEY_EVENT_NONE;
                key_status.click_time = 0;
                need_timer = false;
            }else{
                HAL_KEY_TRACE("key_status DOUBLECLICK-->CLICK1:%d", key_status.key_code);
                key_detected_callback(key_status.key_code, HAL_KEY_EVENT_CLICK);
                key_status.key_code = key_code;
                key_status.event = HAL_KEY_EVENT_CLICK;
                key_status.click_time = hal_sys_timer_get();
                need_timer = true;
            }
        }else{
            HAL_KEY_TRACE("key_status DOUBLECLICK-->CLICK2:%d", key_status.key_code);
            key_detected_callback(key_status.key_code, HAL_KEY_EVENT_CLICK);
            key_status.key_code = key_code;
            key_status.event = HAL_KEY_EVENT_CLICK;
            key_status.click_time = hal_sys_timer_get();
            need_timer = true;
        }

    }

    if (need_timer){
        hal_key_debounce_timer_restart();
        hal_sysfreq_req(HAL_SYSFREQ_USER_KEY, HAL_CMU_FREQ_26M);
    }else{
    	key_detected_callback(HAL_KEY_CODE_NONE, HAL_KEY_EVENT_NONE);
        hal_sysfreq_req(HAL_SYSFREQ_USER_KEY, HAL_CMU_FREQ_32K);
    }
}

#ifndef APP_TEST_MODE
static void hal_key_initializationhandler(void)
{
    bool need_timer = false;
    bool init_done = false;
    static uint32_t init_cnt = 0;
	static enum HAL_KEY_EVENT_T report_event;

    if (init_cnt != 0xffffffff)
        init_cnt++;

    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
    {
        uint32_t v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET);
        HAL_KEY_TRACE("hal_key_initializationhandler event:%d cnt:%d val:%d", key_status.pwr_key.event, init_cnt,(v & IOMUXIP_MUX40_PWRKEY_VAL_MASK)?1:0);

        if (key_status.pwr_key.event == HAL_KEY_EVENT_NONE||
            key_status.pwr_key.event == HAL_KEY_EVENT_DEBOUNCE){
            need_timer = true;
            init_done = false;
            if (init_cnt >= (KEY_INIT_DOWN_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS)){
                if (v & IOMUXIP_MUX40_PWRKEY_VAL_MASK){
                    key_status.pwr_key.event = HAL_KEY_EVENT_DOWN;
                    HAL_KEY_TRACE("key_status.pwr_key init key down");
                    report_event = HAL_KEY_EVENT_INITDOWN;
                    key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITDOWN);
                }else{
                    key_status.pwr_key.event = HAL_KEY_EVENT_DITHERING;
                }
            }
        }else if (key_status.pwr_key.event == HAL_KEY_EVENT_DITHERING){
                HAL_KEY_TRACE("key_status.pwr_initvkey DITHERING");
                need_timer = false;
                init_done = true;
                report_event = HAL_KEY_EVENT_INITUP;
                key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITUP);
        }else{
                if (v & IOMUXIP_MUX40_PWRKEY_VAL_MASK){
                    need_timer = true;
                    init_done = false;
                    if (init_cnt >= (KEY_INIT_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS) && (report_event != HAL_KEY_EVENT_INITLONGPRESS)){
                       key_status.pwr_key.event = HAL_KEY_EVENT_INITLONGPRESS;
                       HAL_KEY_TRACE("key_status.pwr_key init longpress");
                       report_event = HAL_KEY_EVENT_INITLONGPRESS;
                       key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITLONGPRESS);
                    }
                    if (init_cnt >= ((KEY_INIT_LONGLONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS)+(KEY_INIT_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS)) &&
                        (report_event != HAL_KEY_EVENT_INITLONGLONGPRESS)){
                       key_status.pwr_key.event = HAL_KEY_EVENT_INITLONGLONGPRESS;
                       HAL_KEY_TRACE("key_status.pwr_key init longlongpress");
                       report_event = HAL_KEY_EVENT_INITLONGLONGPRESS;
                       key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITLONGLONGPRESS);
                    }
                }else{
                    init_done = true;
                    need_timer = false;
                    if (init_cnt < (KEY_INIT_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS) && (key_status.pwr_key.event == HAL_KEY_EVENT_DOWN)){
                        report_event = HAL_KEY_EVENT_INITLONGLONGPRESS;
                        key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITUP);
                        HAL_KEY_TRACE("key_status.pwr_key init up");
                    }
                    HAL_KEY_TRACE("key_status.pwr_key reset");
                }
            }
    }
    else
    {
        if (key_status.pwr_key.event == HAL_KEY_EVENT_NONE){
            need_timer = true;
            init_done = false;

            if (init_cnt >= (KEY_INIT_DOWN_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS)){
                key_status.pwr_key.event = HAL_KEY_EVENT_DOWN;
                HAL_KEY_TRACE("key_status.pwr_key init key down");
    			report_event = HAL_KEY_EVENT_INITDOWN;
                key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITDOWN);
            }
        }else if (key_status.pwr_key.event == HAL_KEY_EVENT_DITHERING){
                HAL_KEY_TRACE("key_status.pwr_key DITHERING");
    			report_event = HAL_KEY_EVENT_INITUP;
                key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITUP);
                need_timer = false;
                init_done = true;
        }else if (key_status.pwr_key.event == HAL_KEY_EVENT_DOWN){
            need_timer = true;
            init_done = false;
            if (init_cnt >= (KEY_INIT_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS) && (report_event != HAL_KEY_EVENT_INITLONGPRESS)){
               key_status.pwr_key.event = HAL_KEY_EVENT_INITLONGPRESS;
               HAL_KEY_TRACE("key_status.pwr_key init longpress");
    		   report_event = HAL_KEY_EVENT_INITLONGPRESS;
               key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITLONGPRESS);
            }
        }else if (key_status.pwr_key.event == HAL_KEY_EVENT_INITLONGPRESS){
            need_timer = true;
            init_done = false;
    		if (init_cnt >= ((KEY_INIT_LONGLONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS)+(KEY_INIT_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS)) &&
    			(report_event != HAL_KEY_EVENT_INITLONGLONGPRESS)){
    		   key_status.pwr_key.event = HAL_KEY_EVENT_INITLONGLONGPRESS;
    		   HAL_KEY_TRACE("key_status.pwr_key init longpress");
    		   report_event = HAL_KEY_EVENT_INITLONGLONGPRESS;
    		   key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITLONGLONGPRESS);
    		}
        }else if (key_status.pwr_key.event == HAL_KEY_EVENT_UP){
            if (init_cnt < (KEY_INIT_LONGPRESS_THRESHOLD_MS/KEY_CHECKER_INTERVAL_MS) && (init_cnt!=0xffffffff)){
                HAL_KEY_TRACE("key_status.pwr_key init up");
    			report_event = HAL_KEY_EVENT_INITUP;
                key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITUP);
            }
            need_timer = false;
            init_done = true;
        }else{
            need_timer = true;
            init_done = false;
        }
    }
    if (need_timer){
        hal_key_debounce_timer_restart();
        hal_sysfreq_req(HAL_SYSFREQ_USER_KEY, HAL_CMU_FREQ_26M);
    }else{
        hal_sysfreq_req(HAL_SYSFREQ_USER_KEY, HAL_CMU_FREQ_32K);
    }

    if (init_done){
        HAL_KEY_TRACE("key init done");
		report_event = HAL_KEY_EVENT_INITFINISHED;
		key_detected_callback(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITFINISHED);

        hwtimer_stop(debounce_timer);
        hwtimer_free(debounce_timer);
        debounce_timer = hwtimer_alloc((HWTIMER_CALLBACK_T)hal_key_debouncehandler, NULL);
        hal_pwrkey_reset();
        hal_key_enable_allint();
    }
    else
    {
        if(hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_2)
            hal_pwrkey_enable_bothedge_int();
    }
}
#endif // !APP_TEST_MODE

static enum HAL_KEY_CODE_T hal_adckey_findkey(uint16_t volt)
{
    enum HAL_KEY_CODE_T key_code = HAL_KEY_CODE_NUM;

    uint16_t index = 0;

    if ( CFG_HW_ADCKEY_ADC_KEYVOLT_BASE<volt && volt<CFG_HW_ADCKEY_ADC_MAXVOLT){
        for(index=0; index<CFG_HW_ADCKEY_NUMBER; index++)
        {
            if (volt <= ADCKEY_VOLT_TABLE[index])
            {
                key_code = CFG_HW_ADCKEY_MAP_TABLE[index];
                break;
            }
        }
    }

    return key_code;
}

static void hal_adckey_irqhandler(void * ptr)
{
    static enum HAL_KEY_CODE_T pre_keycode = HAL_KEY_CODE_NUM;
    static uint32_t debounce_cnt = 0;
    enum HAL_KEY_CODE_T cur_keycode = HAL_KEY_CODE_NUM;
    unsigned short read_val = *(unsigned short *)ptr;

    hal_key_disable_allint();


    if(read_val & (1<<7)){
        //gpadc chan7
        debounce_cnt++;
        key_reg_read(0x7f,&read_val);
        cur_keycode = hal_adckey_findkey(read_val&0x03FF);
        HAL_KEY_TRACE("irq,adckey,volt:%d", read_val&0x03FF);
        HAL_KEY_TRACE("irq,cur:%d pre:%d adc_key.volt:%d", cur_keycode, pre_keycode, read_val&0x03FF);
        if((debounce_cnt < 3)){
            if (cur_keycode == HAL_KEY_CODE_NUM){
                HAL_KEY_TRACE("1 irq,adckey DITHERING");
                pre_keycode = HAL_KEY_CODE_NUM;
                key_status.adc_key.event = HAL_KEY_EVENT_DITHERING;
            }else{
                if (0xffff == key_status.adc_key.volt){
                    key_status.adc_key.volt = read_val&0x03FF;
                    pre_keycode = cur_keycode;
                    HAL_KEY_TRACE("debounce again");
                }
                if(cur_keycode != pre_keycode){
                    HAL_KEY_TRACE("2 irq,adckey DITHERING");
                    pre_keycode = HAL_KEY_CODE_NUM;
                    key_status.adc_key.event = HAL_KEY_EVENT_DITHERING;
                }
            }
        }else{
            if ((pre_keycode != HAL_KEY_CODE_NUM)&&
                (pre_keycode != cur_keycode)){
                //key release
                HAL_KEY_TRACE("irq,adckey release");
                key_status.adc_key.event = HAL_KEY_EVENT_UP;
                debounce_cnt = 0;
                pre_keycode = HAL_KEY_CODE_NUM;
            }
        }
    }
    if(read_val & (1<<10)){
        //key press
        HAL_KEY_TRACE("irq,adckey press");
        debounce_cnt = 0;
        pre_keycode = HAL_KEY_CODE_NUM;
        key_status.adc_key.event = HAL_KEY_EVENT_DEBOUNCE;
        hal_key_debounce_timer_restart();
    }
    if(read_val & (1<<11)){
        //key err0
        HAL_KEY_TRACE("irq,key err0");
        key_status.adc_key.event = HAL_KEY_EVENT_DITHERING;
        hal_key_debounce_timer_restart();
    }
    if(read_val & (1<<12)){
        //key err1
        HAL_KEY_TRACE("irq,key err1");
        key_status.adc_key.event = HAL_KEY_EVENT_DITHERING;
        hal_key_debounce_timer_restart();
    }
}

static void hal_adckey_open(void)
{
    uint16_t i;
    uint32_t basevolt;

    HAL_KEY_TRACE("%s\n", __func__);

    hal_adckey_reset();

    basevolt = ((CFG_HW_ADCKEY_ADC_MAXVOLT-CFG_HW_ADCKEY_ADC_MINVOLT)/(CFG_HW_ADCKEY_NUMBER+2));

    ADCKEY_VOLT_TABLE[0] = CFG_HW_ADCKEY_ADC_KEYVOLT_BASE + basevolt;

    for(i=1; i<CFG_HW_ADCKEY_NUMBER-1; i++)
    {
        ADCKEY_VOLT_TABLE[i] = ADCKEY_VOLT_TABLE[i-1]+basevolt;
    }
    ADCKEY_VOLT_TABLE[i] = CFG_HW_ADCKEY_ADC_MAXVOLT;

    hal_gpadc_open(HAL_GPADC_CHAN_ADCKEY, HAL_GPADC_ATP_NULL,  hal_adckey_irqhandler);
}

static void hal_adckey_close(void)
{
    HAL_KEY_TRACE("%s\n", __func__);

    hal_adckey_reset();

    hal_adckey_disable_allint();
    hal_gpadc_close(HAL_GPADC_CHAN_ADCKEY);
}

#ifndef APP_TEST_MODE
static void hal_pwrkey_irqhandler(void)
{
#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2){
        uint32_t v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET);
        uint32_t time = hal_sys_timer_get();
        HAL_KEY_TRACE("%s %08x\n", __func__, v);

        hal_key_disable_allint();
        if ((v & (IOMUXIP_MUX40_PWRKEY_DOWN_INT_MASK|IOMUXIP_MUX40_PWRKEY_UP_INT_MASK)) ||
            (v & IOMUXIP_MUX40_PWRKEY_VAL_MASK)){
            HAL_KEY_TRACE("key_status.pwr_key irq down");
            key_status.pwr_key.event = HAL_KEY_EVENT_UP;
            key_status.pwr_key.time = time;
            hal_key_debounce_timer_restart();
        }else{
            HAL_KEY_TRACE("key_status.pwr_key trig DITHERING");
            hal_pwrkey_enable_bothedge_int();
        }
    }else{
	    uint32_t v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET);
	    uint32_t time = hal_sys_timer_get();
	    HAL_KEY_TRACE("%s %08x\n", __func__, v);

	    hal_key_disable_allint();

	    if (v & IOMUXIP_MUX40_PWRKEY_UP_INT_MASK){
	        HAL_KEY_TRACE("key_status.pwr_key irq up");
	        key_status.pwr_key.event = HAL_KEY_EVENT_UP;
	    }
	    hal_key_debounce_timer_restart();
    }
#else // !__POWERKEY_CTRL_ONOFF_ONLY__

    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
    {
        uint32_t v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET);
        uint32_t time = hal_sys_timer_get();
        HAL_KEY_TRACE("%s %08x\n", __func__, v);

        hal_key_disable_allint();
        if ((v & (IOMUXIP_MUX40_PWRKEY_DOWN_INT_MASK|IOMUXIP_MUX40_PWRKEY_UP_INT_MASK)) ||
            (v & IOMUXIP_MUX40_PWRKEY_VAL_MASK)){
            HAL_KEY_TRACE("key_status.pwr_key irq down");
            key_status.pwr_key.event = HAL_KEY_EVENT_DEBOUNCE;
            key_status.pwr_key.time = time;
            hal_key_debounce_timer_restart();
        }else{
            HAL_KEY_TRACE("key_status.pwr_key trig DITHERING");
            hal_pwrkey_enable_bothedge_int();
        }
    }
    else
    {
        uint32_t v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET);
        uint32_t time = hal_sys_timer_get();
        HAL_KEY_TRACE("%s %08x\n", __func__, v);

        hal_key_disable_allint();

        if (v & IOMUXIP_MUX40_PWRKEY_DOWN_INT_MASK){
            HAL_KEY_TRACE("key_status.pwr_key irq down");
            key_status.pwr_key.event = HAL_KEY_EVENT_DEBOUNCE;
            key_status.pwr_key.time = time;
            hal_pwrkey_enable_riseedge_int();
            hal_key_debounce_timer_restart();
        }

        if (v & IOMUXIP_MUX40_PWRKEY_UP_INT_MASK){
            HAL_KEY_TRACE("key_status.pwr_key irq up");
            if ((key_status.pwr_key.event == HAL_KEY_EVENT_DEBOUNCE)&&
                ((time - key_status.pwr_key.time)< KEY_CHECKER_INTERVAL_MS)){
                key_status.pwr_key.event = HAL_KEY_EVENT_DITHERING;
                HAL_KEY_TRACE("key_status.pwr_key trig DITHERING");
            }else{
                key_status.pwr_key.event = HAL_KEY_EVENT_UP;
    //          HAL_KEY_TRACE("key_status.pwr_key trig up");
            }
        }
    }

#endif // !__POWERKEY_CTRL_ONOFF_ONLY__
}

static void hal_pwrkey_open(void)
{
    hal_pwrkey_reset();

    NVIC_SetVector(PWRKEY_IRQn, (uint32_t)hal_pwrkey_irqhandler);
    NVIC_SetPriority(PWRKEY_IRQn, IRQ_PRIORITY_NORMAL);
    NVIC_EnableIRQ(PWRKEY_IRQn);
}

static void hal_pwrkey_close(void)
{
    hal_pwrkey_reset();

    hal_pwrkey_disable_int();

    NVIC_SetVector(PWRKEY_IRQn, (uint32_t)NULL);
    NVIC_DisableIRQ(PWRKEY_IRQn);
}
#endif // !APP_TEST_MODE

static void hal_gpiokey_setirq(enum HAL_GPIO_PIN_T pin, enum HAL_GPIO_IRQ_POLARITY_T porlariry, void (* HAL_GPIO_PIN_IRQ_HANDLER)(enum HAL_GPIO_PIN_T))
{
    struct HAL_GPIO_IRQ_CFG_T gpiocfg;

    if (HAL_GPIO_PIN_IRQ_HANDLER){
        gpiocfg.irq_enable = true;
        gpiocfg.irq_debounce = true;
        gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;
        gpiocfg.irq_porlariry = porlariry;
        gpiocfg.irq_handler = HAL_GPIO_PIN_IRQ_HANDLER;
#if (HAL_GPIOKEY_VALID_LEVEL)
        hal_gpio_pin_set_dir(pin, HAL_GPIO_DIR_IN, 1);
#else
        hal_gpio_pin_set_dir(pin, HAL_GPIO_DIR_IN, 0);
#endif
        hal_gpio_setup_irq(pin, &gpiocfg);
    }else{
        gpiocfg.irq_enable = false;
        gpiocfg.irq_debounce = false;
        gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;
        gpiocfg.irq_porlariry = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
        gpiocfg.irq_handler = NULL;
        hal_gpio_pin_set_dir(pin, HAL_GPIO_DIR_IN, 0);
        hal_gpio_setup_irq(pin, &gpiocfg);
    }
}

static enum HAL_KEY_CODE_T hal_gpiokey_findkey(enum HAL_GPIO_PIN_T pin)
{
    uint8_t i = 0;

    for (i = 0; i<CFG_HW_GPIOKEY_NUM; i++){
        if ((enum HAL_GPIO_PIN_T)cfg_hw_gpio_key_cfg[i].key_config.pin == pin)
            return (cfg_hw_gpio_key_cfg[i].key_code);
    }

    return HAL_KEY_CODE_NUM;
}

static void hal_gpiokey_irqhandler(enum HAL_GPIO_PIN_T pin)
{
    uint8_t gpio_val;

    hal_key_disable_allint();

    gpio_val = hal_gpio_pin_get_val(pin);

    if ((key_status.gpio_key.pin == HAL_GPIO_PIN_NUM) && (gpio_val == CFG_HW_GPIOKEY_DOWN_LEVEL)){
        HAL_KEY_TRACE("key_status.gpio_key trig");
        // any key press, try to wait key release
        key_status.gpio_key.pin = pin;
        key_status.gpio_key.event = HAL_KEY_EVENT_DEBOUNCE;
        key_status.gpio_key.time = hal_sys_timer_get();
        hal_key_debounce_timer_restart();
    }else{
        HAL_KEY_TRACE("key_status.gpio_key trig DITHERING");
        hal_gpiokey_enable_allint();
    }
}

enum HAL_KEY_EVENT_T hal_bt_key_read(enum HAL_KEY_CODE_T code)
{
	uint8_t gpio_val;
	uint8_t i;

	if (code == HAL_KEY_CODE_PWR){
		uint32_t v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET);
		if (v & IOMUXIP_MUX40_PWRKEY_VAL_MASK)
			return HAL_KEY_EVENT_DOWN;
		else
			return HAL_KEY_EVENT_UP;
	}else{
		for(i = 0; i < CFG_HW_GPIOKEY_NUM; i++) {
			if(cfg_hw_gpio_key_cfg[i].key_code == code){
				gpio_val = hal_gpio_pin_get_val(cfg_hw_gpio_key_cfg[i].key_config.pin);
				if(gpio_val == CFG_HW_GPIOKEY_UP_LEVEL)
					return HAL_KEY_EVENT_UP;
				else
					return HAL_KEY_EVENT_DOWN;
			}
		}
	}
	return HAL_KEY_EVENT_NONE;
}

static void hal_gpiokey_open(void)
{
    uint8_t i;
    HAL_KEY_TRACE("%s\n", __func__);

    hal_gpiokey_reset();

    for (i =0; i< CFG_HW_GPIOKEY_NUM; i++){
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)(&(cfg_hw_gpio_key_cfg[i].key_config)), 1);
    }
}

static void hal_gpiokey_close(void)
{
    uint8_t i;
    HAL_KEY_TRACE("%s\n", __func__);

    hal_gpiokey_reset();

    hal_gpiokey_disable_allint();

    for (i =0; i< CFG_HW_GPIOKEY_NUM; i++){
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)(&(cfg_hw_gpio_key_cfg[i].key_config)), 1);
    }
}

#ifndef APP_TEST_MODE
static bool BOOT_TEXT_SRAM_LOC hal_key_isvalid(void)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
    {
        uint32_t v;
        uint32_t lock = int_lock();
        v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET);
        int_unlock(lock);
        if (v & IOMUXIP_MUX40_PWRKEY_VAL_MASK)
            return true;
        else
            return false;
    }
    else
    {
        uint32_t v;
        struct CMU_T * const cmu = (struct CMU_T *)0x40000000;

        uint32_t lock = int_lock();
        uint32_t wakeup_cfg = cmu->WAKEUP_CLK_CFG;
        uint32_t sys_clk = cmu->SYS_CLK;
        cmu->WAKEUP_CLK_CFG = 0;
        hal_pwrkey_enable_bothedge_int();
        hal_cmu_reset_pulse(HAL_CMU_MOD_O_SLEEP);
        hal_sys_timer_delay(MS_TO_TICKS(2));
        v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET);
        cmu->WAKEUP_CLK_CFG = 0x1000;
        hal_sys_timer_delay(MS_TO_TICKS(2));
        cmu->WAKEUP_CLK_CFG = wakeup_cfg;
        cmu->SYS_CLK = sys_clk;
        hal_sys_timer_delay(MS_TO_TICKS(2));
        int_unlock(lock);

        if (!(v & (IOMUXIP_MUX40_PWRKEY_DOWN_INT_MASK|IOMUXIP_MUX40_PWRKEY_UP_INT_MASK))){
            return false;
        }
        hal_pwrkey_disable_int();
        return true;
    }
}
#endif

int hal_key_open(int checkPwrKey, int (* cb)(uint8_t, uint8_t))
{
    int nRet = 0;
    uint32_t lock;

    key_detected_callback = cb;

    key_status.key_code = HAL_KEY_CODE_NUM;
    key_status.event = HAL_KEY_EVENT_NONE;
    key_status.click_time = 0;

    lock = int_lock();

#ifdef APP_TEST_MODE

    hal_adckey_open();
    hal_gpiokey_open();
    if (debounce_timer == NULL)
        debounce_timer = hwtimer_alloc((HWTIMER_CALLBACK_T)hal_key_debouncehandler , NULL);
    hal_key_enable_allint();

#else // !APP_TEST_MODE

    if (checkPwrKey && !hal_key_isvalid()){
        TRACE("key_status.pwr_key init DITHERING");
        nRet = -1;
        goto _exit;
    }
    hal_pwrkey_open();
    hal_adckey_open();
    hal_gpiokey_open();

#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
    if (checkPwrKey) {
        if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
        {
            hal_key_disable_allint();
            hal_pwrkey_reset();
        }
        else
        {
            hal_pwrkey_enable_bothedge_int();
        }
        if (debounce_timer == NULL)
            debounce_timer = hwtimer_alloc((HWTIMER_CALLBACK_T)hal_key_initializationhandler, NULL);
        hwtimer_start(debounce_timer, KEY_CHECKER_INTERVAL_MS);
    } else
#endif
    {
        if (debounce_timer == NULL)
            debounce_timer = hwtimer_alloc((HWTIMER_CALLBACK_T)hal_key_debouncehandler , NULL);
        hal_key_enable_allint();
    }

#endif // !APP_TEST_MODE

    goto _exit; // Avoid compiler warnings
_exit:
    int_unlock(lock);
    return nRet;
}

int hal_key_close(void)
{
    key_status.key_code = HAL_KEY_CODE_NUM;
    key_status.event = HAL_KEY_EVENT_NONE;
    key_status.click_time = 0;

    hal_key_disable_allint();
    if (debounce_timer != NULL){
        hwtimer_stop(debounce_timer);
        hwtimer_free(debounce_timer);
    }
    key_detected_callback = NULL;
#ifndef APP_TEST_MODE
    hal_pwrkey_close();
#endif
    hal_adckey_close();
    hal_gpiokey_close();
    return 0;
}

