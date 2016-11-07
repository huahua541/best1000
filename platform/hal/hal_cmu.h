#ifndef __HAL_CMU_H__
#define __HAL_CMU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

enum HAL_CMU_MOD_ID_T {
    // HCLK/HRST
    HAL_CMU_MOD_H_MCU,          // 0
    HAL_CMU_MOD_H_ROM,          // 1
    HAL_CMU_MOD_H_RAM0,         // 2
    HAL_CMU_MOD_H_RAM1,         // 3
    HAL_CMU_MOD_H_AHB0,         // 4
    HAL_CMU_MOD_H_AHB1,         // 5
    HAL_CMU_MOD_H_AH2H,         // 6
    HAL_CMU_MOD_H_AH2H_BT,      // 7
    HAL_CMU_MOD_H_ADMA,         // 8
    HAL_CMU_MOD_H_GDMA,         // 9
    HAL_CMU_MOD_H_PSRAM,        // 10
    HAL_CMU_MOD_H_FLASH,        // 11
    HAL_CMU_MOD_H_SDIO,         // 12
    HAL_CMU_MOD_H_SDMMC,        // 13
    HAL_CMU_MOD_H_USBC,         // 14
    HAL_CMU_MOD_H_DPDRX,        // 15
    HAL_CMU_MOD_H_DPDTX,        // 16
    // PCLK/PRST
    HAL_CMU_MOD_P_CMU,          // 17
    HAL_CMU_MOD_P_GPIO,         // 18
    HAL_CMU_MOD_P_GPIO_INT,     // 19
    HAL_CMU_MOD_P_WDT,          // 20
    HAL_CMU_MOD_P_PWM,          // 21
    HAL_CMU_MOD_P_TIMER,        // 22
    HAL_CMU_MOD_P_I2C,          // 23
    HAL_CMU_MOD_P_SPI,          // 24
    HAL_CMU_MOD_P_SLCD,         // 25
    HAL_CMU_MOD_P_UART0,        // 26
    HAL_CMU_MOD_P_UART1,        // 27
    HAL_CMU_MOD_P_CODEC,        // 28
    HAL_CMU_MOD_P_PCM,          // 29
    HAL_CMU_MOD_P_I2S,          // 30
    HAL_CMU_MOD_P_SPI_ITN,      // 31
    HAL_CMU_MOD_P_IOMUX,        // 32
    HAL_CMU_MOD_P_SPDIF0,       // 33
    HAL_CMU_MOD_P_GLOBAL,       // 34
    // OCLK/ORST
    HAL_CMU_MOD_O_SLEEP,        // 35
    HAL_CMU_MOD_O_FLASH,        // 36
    HAL_CMU_MOD_O_PSRAM,        // 37
    HAL_CMU_MOD_O_USB,          // 38
    HAL_CMU_MOD_O_SDMMC,        // 39
    HAL_CMU_MOD_O_SDIO,         // 40
    HAL_CMU_MOD_O_WDT,          // 41
    HAL_CMU_MOD_O_BT,           // 42
    HAL_CMU_MOD_O_TIMER,        // 43
    HAL_CMU_MOD_O_SPI,          // 44
    HAL_CMU_MOD_O_SLCD,         // 45
    HAL_CMU_MOD_O_UART0,        // 46
    HAL_CMU_MOD_O_UART1,        // 47
    HAL_CMU_MOD_O_CODEC_AD,     // 48
    HAL_CMU_MOD_O_CODEC_DA,     // 49
    HAL_CMU_MOD_O_I2S,          // 50
    HAL_CMU_MOD_O_SPDIF0,       // 51
    HAL_CMU_MOD_O_PCM,          // 52
    HAL_CMU_MOD_O_I2C,          // 53
    HAL_CMU_MOD_O_SPI_ITN,      // 54
    HAL_CMU_MOD_O_GPIO,         // 55
    HAL_CMU_MOD_O_PWM0,         // 56
    HAL_CMU_MOD_O_PWM1,         // 57
    HAL_CMU_MOD_O_PWM2,         // 58
    HAL_CMU_MOD_O_PWM3,         // 59
    HAL_CMU_MOD_O_BTCPU,        // 60

    HAL_CMU_MOD_QTY,

    HAL_CMU_MOD_GLOBAL = HAL_CMU_MOD_P_GLOBAL,
    HAL_CMU_MOD_BT = HAL_CMU_MOD_O_BT,
    HAL_CMU_MOD_BTCPU = HAL_CMU_MOD_O_BTCPU,
};

enum HAL_CMU_CLK_STATUS_T {
    HAL_CMU_CLK_DISABLED,
    HAL_CMU_CLK_ENABLED,
};

enum HAL_CMU_CLK_MODE_T {
    HAL_CMU_CLK_AUTO,
    HAL_CMU_CLK_MANUAL,
};

enum HAL_CMU_RST_STATUS_T {
    HAL_CMU_RST_SET,
    HAL_CMU_RST_CLR,
};

enum HAL_CMU_TIMER_ID_T {
    HAL_CMU_TIMER_ID_1,
    HAL_CMU_TIMER_ID_2,
};

enum HAL_CMU_FREQ_T {
    HAL_CMU_FREQ_32K,
    HAL_CMU_FREQ_13M,
    HAL_CMU_FREQ_26M,
    HAL_CMU_FREQ_52M,
    HAL_CMU_FREQ_78M,
    HAL_CMU_FREQ_104M,
    HAL_CMU_FREQ_208M,

    HAL_CMU_FREQ_QTY
};

enum HAL_CMU_PLL_T {
    HAL_CMU_PLL_AUD,
    HAL_CMU_PLL_USB,

    HAL_CMU_PLL_QTY
};

enum HAL_CMU_CODEC_FREQ_T {
    HAL_CMU_CODEC_FREQ_44_1K,
    HAL_CMU_CODEC_FREQ_48K,

    HAL_CMU_CODEC_FREQ_QTY
};

enum HAL_CMU_PLL_USER_T {
    HAL_CMU_PLL_USER_INIT,
    HAL_CMU_PLL_USER_SYS,
    HAL_CMU_PLL_USER_AUD,
    HAL_CMU_PLL_USER_USB,
    HAL_CMU_PLL_USER_SWITCH,

    HAL_CMU_PLL_USER_QTY
};

enum HAL_CMU_PERIPH_FREQ_T {
    HAL_CMU_PERIPH_FREQ_26M,
    HAL_CMU_PERIPH_FREQ_52M,

    HAL_CMU_PERIPH_FREQ_QTY
};

enum HAL_CMU_PWM_ID_T {
    HAL_CMU_PWM_ID_0,
    HAL_CMU_PWM_ID_1,
    HAL_CMU_PWM_ID_2,
    HAL_CMU_PWM_ID_3,

    HAL_CMU_PWM_ID_QTY
};

enum HAL_CMU_CLOCK_OUT_ID_T {
    HAL_CMU_CLOCK_OUT_32K,
    HAL_CMU_CLOCK_OUT_6M5,
    HAL_CMU_CLOCK_OUT_13M,
    HAL_CMU_CLOCK_OUT_26M,
    HAL_CMU_CLOCK_OUT_52M,
    HAL_CMU_CLOCK_OUT_SYSPLL,
    HAL_CMU_CLOCK_OUT_48M,
    HAL_CMU_CLOCK_OUT_60M,
    HAL_CMU_CLOCK_OUT_CODEC_ADC,
    HAL_CMU_CLOCK_OUT_CODEC_HIGH,
    HAL_CMU_CLOCK_OUT_HCLK,
    HAL_CMU_CLOCK_OUT_PCLK,
    HAL_CMU_CLOCK_OUT_CODEC_ADC_IN,
    HAL_CMU_CLOCK_OUT_BT,

    HAL_CMU_CLOCK_OUT_QTY
};

int hal_cmu_clock_enable(enum HAL_CMU_MOD_ID_T id);

int hal_cmu_clock_disable(enum HAL_CMU_MOD_ID_T id);

enum HAL_CMU_CLK_STATUS_T hal_cmu_clock_get_status(enum HAL_CMU_MOD_ID_T id);

int hal_cmu_clock_set_mode(enum HAL_CMU_MOD_ID_T id, enum HAL_CMU_CLK_MODE_T mode);

enum HAL_CMU_CLK_MODE_T hal_cmu_clock_get_mode(enum HAL_CMU_MOD_ID_T id);

int hal_cmu_reset_set(enum HAL_CMU_MOD_ID_T id);

int hal_cmu_reset_clear(enum HAL_CMU_MOD_ID_T id);

enum HAL_CMU_RST_STATUS_T hal_cmu_reset_get_status(enum HAL_CMU_MOD_ID_T id);

int hal_cmu_reset_pulse(enum HAL_CMU_MOD_ID_T id);

int hal_cmu_timer_set_div(enum HAL_CMU_TIMER_ID_T id, uint32_t div);

void hal_cmu_timer_select_fast(void);

void hal_cmu_timer_select_slow(void);

int hal_cmu_uart0_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq);

int hal_cmu_uart1_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq);

int hal_cmu_spi_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq);

int hal_cmu_slcd_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq);

int hal_cmu_sdio_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq);

int hal_cmu_sdmmc_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq);

int hal_cmu_i2c_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq);

int hal_cmu_ispi_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq);

int hal_cmu_pwm_set_freq(enum HAL_CMU_PWM_ID_T id, uint32_t freq);

int hal_cmu_flash_set_freq(enum HAL_CMU_FREQ_T freq);

int hal_cmu_mem_set_freq(enum HAL_CMU_FREQ_T freq);

int hal_cmu_sys_set_freq(enum HAL_CMU_FREQ_T freq);

enum HAL_CMU_FREQ_T hal_cmu_sys_get_freq(void);

int hal_cmu_flash_select_pll(enum HAL_CMU_PLL_T pll);

int hal_cmu_mem_select_pll(enum HAL_CMU_PLL_T pll);

int hal_cmu_sys_select_pll(enum HAL_CMU_PLL_T pll);

int hal_cmu_pll_enable(enum HAL_CMU_PLL_T pll, enum HAL_CMU_PLL_USER_T user);

int hal_cmu_pll_disable(enum HAL_CMU_PLL_T pll, enum HAL_CMU_PLL_USER_T user);

int hal_cmu_sys_get_pll_status(enum HAL_CMU_PLL_T *pll, uint32_t *status);

void hal_cmu_audio_resample_enable(void);

void hal_cmu_audio_resample_disable(void);

int hal_cmu_get_audio_resample_status(void);

int hal_cmu_codec_adc_set_freq(enum HAL_CMU_CODEC_FREQ_T freq);

int hal_cmu_codec_dac_set_freq(enum HAL_CMU_CODEC_FREQ_T freq);

uint32_t hal_cmu_codec_adc_get_div(void);

uint32_t hal_cmu_codec_dac_get_div(void);

void hal_cmu_anc_enable(void);

void hal_cmu_anc_disable(void);

int hal_cmu_anc_get_status(void);

void hal_cmu_codec_clock_enable(void);

void hal_cmu_codec_clock_disable(void);

void hal_cmu_i2s_clock_enable(void);

void hal_cmu_i2s_clock_disable(void);

void hal_cmu_i2s_set_slave_mode(void);

void hal_cmu_i2s_set_master_mode(void);

int hal_cmu_i2s_set_div(uint32_t div);

int hal_cmu_pcm_set_div(uint32_t div);

void hal_cmu_usb_set_device_mode(void);

void hal_cmu_usb_set_host_mode(void);

int hal_cmu_clock_out_enable(enum HAL_CMU_CLOCK_OUT_ID_T id);

void hal_cmu_clock_out_disable(void);

void hal_cmu_write_lock(void);

void hal_cmu_write_unlock(void);

void hal_cmu_sys_reboot(void);

void hal_cmu_jtag_enable(void);

void hal_cmu_jtag_disable(void);

void hal_cmu_simu_init(void);

void hal_cmu_simu_pass(void);

void hal_cmu_simu_fail(void);

void hal_cmu_simu_tag(uint8_t shift);

void hal_cmu_simu_setval(uint32_t val);

void hal_cmu_low_freq_mode_enable(void);

void hal_cmu_low_freq_mode_disable(void);

void hal_cmu_rom_setup(void);

void hal_cmu_fpga_setup(void);

void hal_cmu_setup(void);

#ifdef __cplusplus
}
#endif

#endif

