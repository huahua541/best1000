#include "plat_types.h"
#include "hal_psram.h"
#include "hal_psramip.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_cmu.h"

//#define PSRAM_HISEPEED 1

#define HAL_PSRAM_YES 1
#define HAL_PSRAM_NO 0

#define HAL_PSRAM_CMD_REG_READ 0x40
#define HAL_PSRAM_CMD_REG_WRITE 0xc0
#define HAL_PSRAM_CMD_RAM_READ 0x00
#define HAL_PSRAM_CMD_RAM_WRITE 0x80

struct HAL_Psram_Context psram_ctx[HAL_PSRAM_ID_NUM];

static const char * const invalid_drv = "psram drv invalid";

#define DIGITAL_REG(a) *(volatile uint32_t *)(a)

static uint32_t _psram_get_reg_base(enum HAL_PSRAM_ID_T id) 
{
    switch(id) {
        case HAL_PSRAM_ID_0:
        default:
            return 0x40150000;
            break;
    }
}

static void _psram_exitsleep_onprocess_wait(enum HAL_PSRAM_ID_T id) 
{
    uint32_t reg_base = 0;
    reg_base = _psram_get_reg_base(id);
    while (psramip_r_exit_sleep_onprocess(reg_base));

    while (!psramip_r_sleep_wakeup_state(reg_base));
}
static void _psram_busy_wait(enum HAL_PSRAM_ID_T id) 
{
    uint32_t reg_base = 0;
    reg_base = _psram_get_reg_base(id);
    while (psramip_r_busy(reg_base));
}
static void _psram_div(enum HAL_PSRAM_ID_T id) 
{
    /* TODO */
}
/* hal api */
uint8_t hal_psram_open(enum HAL_PSRAM_ID_T id, struct HAL_PSRAM_CONFIG_T *cfg)
{
    uint32_t div = 0, reg_base = 0;
    //uint32_t psram_id = 0;
    reg_base = _psram_get_reg_base(id);

    /* over write config */
    if (cfg->override_config) {
        /* div */
        _psram_div(cfg->div);
    }
    else {
        div = cfg->source_clk/cfg->speed;
        _psram_div(div);
    }

    /* 0. dqs config */
    psramip_w_dqs_rd_sel(reg_base, cfg->dqs_rd_sel);
    psramip_w_dqs_wr_sel(reg_base, cfg->dqs_wr_sel);

    /* 1. high speed mode */
    if (cfg->speed >= HAL_PSRAM_SPEED_50M)
        psramip_w_high_speed_enable(reg_base, HAL_PSRAM_YES);
    else
        psramip_w_high_speed_enable(reg_base, HAL_PSRAM_NO);

    _psram_busy_wait(id);

    /* 2. wait calib done or FIXME timeout */
    psramip_w_enable_and_trigger_calib(reg_base);
    while (!psramip_r_calibst(reg_base));

    psramip_w_wrap_mode_enable(reg_base, HAL_PSRAM_YES);
    //psramip_w_wrap_mode_enable(reg_base, HAL_PSRAM_NO);

    //psramip_w_32bytewrap_mode(reg_base);
    psramip_w_1kwrap_mode(reg_base);
#if 0
    /* psram device register read 1 or 2 or 3 */
    psramip_w_acc_size(reg_base, 1);
    psramip_w_cmd_addr(reg_base, HAL_PSRAM_CMD_REG_READ, 2);
    _psram_busy_wait(id);
    psram_id = psramip_r_rx_fifo(reg_base);

    uart_printf("psram id 0x%x\n", psram_id);
#endif

    return 0;
}

void hal_psram_suspend(enum HAL_PSRAM_ID_T id)
{
    uint32_t reg_base = 0;
    reg_base = _psram_get_reg_base(id);

    psramip_w_acc_size(reg_base, 1);
    psramip_w_tx_fifo(reg_base, 0xf0);
    psramip_w_cmd_addr(reg_base, HAL_PSRAM_CMD_REG_WRITE, 6);
    _psram_busy_wait(id);
}

void hal_psram_resume(enum HAL_PSRAM_ID_T id)
{
    uint32_t reg_base = 0;
    reg_base = _psram_get_reg_base(id);

    psramip_w_exit_sleep(reg_base);

    _psram_exitsleep_onprocess_wait(id);
}

void hal_psram_reg_dump(enum HAL_PSRAM_ID_T id) 
{
#if 0
    uint32_t reg_base = 0;
    uint32_t psram_id = 0;
    reg_base = _psram_get_reg_base(id);

    /* psram device register read 1 or 2 or 3 */
    psramip_w_acc_size(reg_base, 1);
    psramip_w_cmd_addr(reg_base, HAL_PSRAM_CMD_REG_READ, 2);
    _psram_busy_wait(id);
    psram_id = psramip_r_rx_fifo(reg_base);

    uart_printf("psram id 0x%x\n", psram_id);
#endif
}

 void hal_psram_init(unsigned char dll_speed)
{
	unsigned int data32 = 0;
    hal_cmu_mem_set_freq(HAL_CMU_FREQ_26M);

#if PSRAM_HISEPEED
    DIGITAL_REG(0x4001f02c) |= (1 << 16);
    DIGITAL_REG(0x4001f03c) |= (1 << 1);

    DIGITAL_REG(0x4001f038) = 0x32383533;
    DIGITAL_REG(0x4001f03c) = 0x3ffffffc;
    DIGITAL_REG(0x4001f03c) |= (1<<1)|(1<<30);
	
    hal_sys_timer_delay(MS_TO_TICKS(10));
    DIGITAL_REG(0x4001f03c) &= ~(1<<0);
    hal_sys_timer_delay(MS_TO_TICKS(10));
    DIGITAL_REG(0x40000090) = 0x0001ffff;
    hal_sys_timer_delay(MS_TO_TICKS(10));

    while (!((DIGITAL_REG(0x40000090)) & (0x1<<20)));
    hal_sys_timer_delay(MS_TO_TICKS(10));
	
    //ramp=1k select low speed
    DIGITAL_REG(0x4015002C)=0x0000000d | 0x2;
    //tx/rx Phase
    DIGITAL_REG(0x40150024)=0x55000663;
#else   
    // open psram phy power
	DIGITAL_REG(0x4001f02c) |= (1 << 16);
    //init psram
    //ramp=1k select low speed
    DIGITAL_REG(0x4015002C)=0x0000000c;
    //tx/rx Phase
    DIGITAL_REG(0x40150024)=0x55000223;
#endif 
//bga no psram.
	hal_psram_sleep();

}

 void hal_psram_wakeup_init(void)
{
	unsigned char i = 100;
#if PSRAM_HISEPEED
	DIGITAL_REG(0x4001f02c) |= (1 << 16);

	while(i-- > 0){
		__asm("nop");
		__asm("nop");
		__asm("nop");
	};

    DIGITAL_REG(0x4001f03c) |= (1<<1)|(1<<30);
	
    hal_sys_timer_delay(MS_TO_TICKS(1));
    DIGITAL_REG(0x4001f03c) &= ~(1<<0);
    hal_sys_timer_delay(MS_TO_TICKS(1));
    DIGITAL_REG(0x40000090) = 0x0001ffff;
    hal_sys_timer_delay(MS_TO_TICKS(1));

    while (!((DIGITAL_REG(0x40000090)) & (0x1<<20)));
    hal_sys_timer_delay(MS_TO_TICKS(1));
	
    //ramp=1k select low speed
    DIGITAL_REG(0x4015002C)=0x0000000d;
    //tx/rx Phase
    DIGITAL_REG(0x40150024)=0x55000663;

#else
    // open psram phy power
	DIGITAL_REG(0x4001f02c) |= (1 << 16);
#endif	
}

 void hal_psram_sleep(void)
{
	 hal_psram_suspend(HAL_PSRAM_ID_0);
     /* dll & phy power off */
	 DIGITAL_REG(0x4001f03c) &= ~(1 << 1);
	 DIGITAL_REG(0x4001f02c) &= ~(1 << 16);
}

 void hal_psram_wakeup(void)
{
	unsigned int i = 1;
	hal_psram_resume(HAL_PSRAM_ID_0);
	while(i-- > 0){
		__asm("nop");
		__asm("nop");
		__asm("nop");
	};
	hal_psram_wakeup_init();
}

uint8_t hal_psram_close(enum HAL_PSRAM_ID_T id)
{
    return 0;
}
