#include "cmsis_os.h"
#include "tgt_hardware.h"
#include "hal_gpio.h"
#include "hal_iomux.h"
#include "hal_trace.h"
#include "app_pwl.h"
#include "string.h"

#define APP_PWL_TRACE(s,...)
//TRACE(s, ##__VA_ARGS__)

static void app_pwl_timehandler(void const *param);
 
osTimerDef (APP_PWL_TIMER0, app_pwl_timehandler);                      // define timers
#if	(CFG_HW_PLW_NUM == 2)
osTimerDef (APP_PWL_TIMER1, app_pwl_timehandler);
#endif

struct APP_PWL_T {
    enum APP_PWL_ID_T id;
    struct APP_PWL_CFG_T config;
    uint8_t partidx;
    osTimerId timer;
};

static struct APP_PWL_T app_pwl[APP_PWL_ID_QTY];

static void app_pwl_timehandler(void const *param)
{
    struct APP_PWL_T *pwl = (struct APP_PWL_T *)param;
    struct APP_PWL_CFG_T *cfg = &(pwl->config);
    APP_PWL_TRACE("%s %x",__func__, param);
    
    osTimerStop(pwl->timer);
    
    pwl->partidx++;
    if (cfg->periodic){
        if (pwl->partidx >= cfg->parttotal){
            pwl->partidx = 0;
        }
    }else{
        if (pwl->partidx >= cfg->parttotal){

			//for eb100 by hw.
			hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[pwl->id].pin);
            return;
        }
    }
    
    APP_PWL_TRACE("idx:%d pin:%d lvl:%d", pwl->partidx, cfg_hw_pinmux_pwl[pwl->id].pin, cfg->part[pwl->partidx].level);
    if(cfg->part[pwl->partidx].level){
        hal_gpio_pin_set((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[pwl->id].pin);
    }else{
        hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[pwl->id].pin);
    }    
    osTimerStart(pwl->timer, cfg->part[pwl->partidx].time);
}

int app_pwl_open(void)
{
    uint8_t i;
    APP_PWL_TRACE("%s",__func__);
    for (i=0;i<APP_PWL_ID_QTY;i++){
        app_pwl[i].id = APP_PWL_ID_QTY;
        memset(&(app_pwl[i].config), 0, sizeof(struct APP_PWL_CFG_T));
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&cfg_hw_pinmux_pwl[i], 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[i].pin, HAL_GPIO_DIR_OUT, 0);
    }
    app_pwl[APP_PWL_ID_0].timer = osTimerCreate (osTimer(APP_PWL_TIMER0), osTimerOnce, &app_pwl[APP_PWL_ID_0]);
#if	(CFG_HW_PLW_NUM == 2)    
    app_pwl[APP_PWL_ID_1].timer = osTimerCreate (osTimer(APP_PWL_TIMER1), osTimerOnce, &app_pwl[APP_PWL_ID_1]);
#endif
    return 0;
}

int app_pwl_start(enum APP_PWL_ID_T id)
{
    struct APP_PWL_T *pwl = NULL;
    struct APP_PWL_CFG_T *cfg = NULL;
    

    if (id >= APP_PWL_ID_QTY) {
        return -1;
    }

    APP_PWL_TRACE("%s %d",__func__, id);

    pwl = &app_pwl[id];
    cfg = &(pwl->config);

    if (pwl->id == APP_PWL_ID_QTY){
        return -1;
    }
    
    pwl->partidx = 0;
    if (pwl->partidx >= cfg->parttotal){
        return -1;
    }

    osTimerStop(pwl->timer);

    APP_PWL_TRACE("idx:%d pin:%d lvl:%d", pwl->partidx, cfg_hw_pinmux_pwl[pwl->id].pin, cfg->part[pwl->partidx].level);
    if(cfg->part[pwl->partidx].level){
        hal_gpio_pin_set((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[pwl->id].pin);
    }else{
        hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[pwl->id].pin);
    }       
    osTimerStart(pwl->timer, cfg->part[pwl->partidx].time);
    return 0;
}

int app_pwl_setup(enum APP_PWL_ID_T id, struct APP_PWL_CFG_T *cfg)
{

    if (cfg == NULL || id >= APP_PWL_ID_QTY) {
        return -1;
    }
    APP_PWL_TRACE("%s %d",__func__, id);
    
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[id].pin, HAL_GPIO_DIR_OUT, cfg->startlevel?1:0);
    app_pwl[id].id = id;
    memcpy(&(app_pwl[id].config), cfg, sizeof(struct APP_PWL_CFG_T));
    
    osTimerStop(app_pwl[id].timer);
    
    return 0;
}

int app_pwl_stop(enum APP_PWL_ID_T id)
{

    if (id >= APP_PWL_ID_QTY) {
        return -1;
    }

    osTimerStop(app_pwl[id].timer);
    hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)cfg_hw_pinmux_pwl[id].pin);

    return 0;
}

int app_pwl_close(void)
{
    uint8_t i;
    for (i=0;i<APP_PWL_ID_QTY;i++){
        if (app_pwl[i].id != APP_PWL_ID_QTY)
            app_pwl_stop((enum APP_PWL_ID_T)i);
    }
    return 0;
}


