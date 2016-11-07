#include "hal_cmu.h"
#include "hal_cmu_addr.h"
#include "reg_cmu.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "hal_sleep.h"
#include "hal_sysfreq.h"
#include "hal_analogif.h"
#include "hal_chipid.h"
#include "cmsis.h"

#define HAL_CMU_AUD_PLL_CLOCK_48K       (221184000)
#define HAL_CMU_AUD_PLL_CLOCK_44_1K     (225792000)
#define HAL_CMU_USB_PLL_CLOCK           (240 * 1000 * 1000)

#define HAL_CMU_USB_CLOCK_60M           (60 * 1000 * 1000)
#define HAL_CMU_USB_CLOCK_48M           (48 * 1000 * 1000)

#define HAL_CMU_PWM_FAST_CLOCK          (26 * 1000 * 1000)
#define HAL_CMU_PWM_SLOW_CLOCK          (32 * 1000)

#define HAL_CMU_PLL_LOCKED_TIMEOUT      MS_TO_TICKS(1)

extern uint8_t hal_norflash_init(void);

static struct CMU_T * const cmu = (struct CMU_T *)0x40000000;

static enum HAL_CMU_FREQ_T BOOT_BSS_LOC sys_freq;

static uint8_t BOOT_BSS_LOC pll_user_map[HAL_CMU_PLL_QTY];

STATIC_ASSERT(HAL_CMU_PLL_USER_QTY <= sizeof(pll_user_map[0]) * 8, "Too many PLL users");

#ifdef __AUDIO_RESAMPLE__
static uint8_t aud_resample_en = 0;
#endif

struct CMU_T *hal_cmu_get_address(void)
{
    return cmu;
}

int hal_cmu_clock_enable(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        cmu->HCLK_ENABLE = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        cmu->PCLK_ENABLE = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        cmu->OCLK_ENABLE = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return 0;
}

int hal_cmu_clock_disable(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        cmu->HCLK_DISABLE = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        cmu->PCLK_DISABLE = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        cmu->OCLK_DISABLE = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return 0;
}

enum HAL_CMU_CLK_STATUS_T hal_cmu_clock_get_status(enum HAL_CMU_MOD_ID_T id)
{
    uint32_t status;

    if (id >= HAL_CMU_MOD_QTY) {
        return HAL_CMU_CLK_DISABLED;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        status = cmu->HCLK_ENABLE & (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        status = cmu->PCLK_ENABLE & (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        status = cmu->OCLK_ENABLE & (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return status ? HAL_CMU_CLK_ENABLED : HAL_CMU_CLK_DISABLED;
}

int hal_cmu_clock_set_mode(enum HAL_CMU_MOD_ID_T id, enum HAL_CMU_CLK_MODE_T mode)
{
    __IO uint32_t *reg;
    uint32_t val;
    uint32_t lock;

    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        reg = &cmu->HCLK_MODE;
        val = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        reg = &cmu->PCLK_MODE;
        val = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        reg = &cmu->OCLK_MODE;
        val = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    lock = int_lock();
    if (mode == HAL_CMU_CLK_MANUAL) {
        *reg |= val;
    } else {
        *reg &= ~val;
    }
    int_unlock(lock);

    return 0;
}

enum HAL_CMU_CLK_MODE_T hal_cmu_clock_get_mode(enum HAL_CMU_MOD_ID_T id)
{
    uint32_t mode;

    if (id >= HAL_CMU_MOD_QTY) {
        return HAL_CMU_CLK_DISABLED;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        mode = cmu->HCLK_MODE & (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        mode = cmu->PCLK_MODE & (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        mode = cmu->OCLK_MODE & (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return mode ? HAL_CMU_CLK_MANUAL : HAL_CMU_CLK_AUTO;
}

int hal_cmu_reset_set(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        cmu->HRESET_SET = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        cmu->PRESET_SET = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        cmu->ORESET_SET = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return 0;
}

int hal_cmu_reset_clear(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        cmu->HRESET_CLR = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        cmu->PRESET_CLR = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        cmu->ORESET_CLR = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return 0;
}

enum HAL_CMU_RST_STATUS_T hal_cmu_reset_get_status(enum HAL_CMU_MOD_ID_T id)
{
    uint32_t status;

    if (id >= HAL_CMU_MOD_QTY) {
        return HAL_CMU_RST_SET;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        status = cmu->HRESET_SET & (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        status = cmu->PRESET_SET & (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        status = cmu->ORESET_SET & (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return status ? HAL_CMU_RST_CLR : HAL_CMU_RST_SET;
}

int hal_cmu_reset_pulse(enum HAL_CMU_MOD_ID_T id)
{
    volatile int i;

    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (hal_cmu_reset_get_status(id) == HAL_CMU_RST_SET) {
        return hal_cmu_reset_clear(id);
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        cmu->HRESET_PULSE = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        cmu->PRESET_PULSE = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        cmu->ORESET_PULSE = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }
    // Delay 5+ PCLK cycles (10+ HCLK cycles)
    for (i = 0; i < 3; i++);

    return 0;
}

int hal_cmu_timer_set_div(enum HAL_CMU_TIMER_ID_T id, uint32_t div)
{
    uint32_t lock;

    if (div < 1) {
        return 1;
    }

    div -= 1;
    if ((div & (CMU_TIMER0_DIV_MASK >> CMU_TIMER0_DIV_SHIFT)) != div) {
        return 1;
    }

    lock = int_lock();
    if (id == HAL_CMU_TIMER_ID_1) {
        cmu->TIMER_CLK = SET_BITFIELD(cmu->TIMER_CLK, CMU_TIMER0_DIV, div);
    } else {
        cmu->TIMER_CLK = SET_BITFIELD(cmu->TIMER_CLK, CMU_TIMER1_DIV, div);
    }
    int_unlock(lock);

    return 0;
}

void hal_cmu_timer_select_fast(void)
{
    uint32_t lock;

    lock = int_lock();
    // 6.5M
    cmu->CLK_OUT |= CMU_SEL_TIMER_FAST;
    int_unlock(lock);
}

void hal_cmu_timer_select_slow(void)
{
    uint32_t lock;

    lock = int_lock();
    // 32K
    cmu->CLK_OUT &= ~CMU_SEL_TIMER_FAST;
    int_unlock(lock);
}

#define PERPH_SET_PLL_DIV_FUNC(f, F) \
int hal_cmu_ ##f## _set_pll_div(uint32_t div) \
{ \
    uint32_t lock; \
    if (div < 2) { \
        return 1; \
    } \
    div -= 2; \
    if ((div & (CMU_CFG_DIV_ ##F## _MASK >> CMU_CFG_DIV_ ##F## _SHIFT)) != div) { \
        return 1; \
    } \
    lock = int_lock(); \
    cmu->PERIPH_CLK = (cmu->PERIPH_CLK & ~(CMU_CFG_DIV_ ##F## _MASK | CMU_SEL_ ##F## _PLL)) | \
        CMU_CFG_DIV_ ##F (div) | CMU_SEL_ ##F## _PLL; \
    int_unlock(lock); \
    return 0; \
}

PERPH_SET_PLL_DIV_FUNC(uart0, UART0);
PERPH_SET_PLL_DIV_FUNC(uart1, UART1);
PERPH_SET_PLL_DIV_FUNC(spi, SPI);
PERPH_SET_PLL_DIV_FUNC(slcd, SLCD);
PERPH_SET_PLL_DIV_FUNC(sdio, SDIO);
PERPH_SET_PLL_DIV_FUNC(sdmmc, SDMMC);

#define SYS_SET_FREQ_FUNC(f, F, S) \
int hal_cmu_ ##f## _set_freq(enum HAL_CMU_FREQ_T freq) \
{ \
    uint32_t lock; \
    uint32_t mask; \
    uint32_t value; \
    if (freq >= HAL_CMU_FREQ_QTY) { \
        return 1; \
    } \
    mask = CMU_SEL_32K_ ##F | CMU_SEL_13M_ ##F | CMU_SEL_26M_ ##F | CMU_SEL_52M_ ##F | \
        CMU_BYPASS_DIV_ ##F | CMU_CFG_DIV_ ##F## _MASK; \
    if (freq == HAL_CMU_FREQ_32K) { \
        value = CMU_SEL_32K_ ##F | CMU_SEL_13M_ ##F | CMU_SEL_26M_ ##F | CMU_SEL_52M_ ##F; \
    } else if (freq == HAL_CMU_FREQ_26M) { \
        value = CMU_SEL_26M_ ##F | CMU_SEL_52M_ ##F; \
    } else if (freq == HAL_CMU_FREQ_52M) { \
        value = CMU_SEL_52M_ ##F; \
    } else if (freq == HAL_CMU_FREQ_78M) { \
        value = CMU_CFG_DIV_ ##F(1); \
    } else if (freq == HAL_CMU_FREQ_104M) { \
        value = CMU_CFG_DIV_ ##F(0); \
    } else { \
        value = CMU_BYPASS_DIV_ ##F; \
    } \
    lock = int_lock(); \
    cmu->SYS_CLK = (cmu->SYS_CLK & ~mask) | value; \
    S; \
    int_unlock(lock); \
    return 0; \
}

BOOT_TEXT_SRAM_LOC SYS_SET_FREQ_FUNC(flash, FLASH, {});
SYS_SET_FREQ_FUNC(mem, MEM, {});
SYS_SET_FREQ_FUNC(sys, SYS, {sys_freq = freq;});

enum HAL_CMU_FREQ_T BOOT_TEXT_SRAM_LOC hal_cmu_sys_get_freq(void)
{
    return sys_freq;
}

#define SYS_SEL_PLL_FUNC(f, F) \
int hal_cmu_ ##f## _select_pll(enum HAL_CMU_PLL_T pll) \
{ \
    uint32_t lock; \
    if (pll >= HAL_CMU_PLL_QTY) { \
        return 1; \
    } \
    lock = int_lock(); \
    if (pll == HAL_CMU_PLL_AUD) { \
        cmu->SYS_CLK &= ~ CMU_SEL_PLLUSB_ ##F; \
    } else { \
        cmu->SYS_CLK |= CMU_SEL_PLLUSB_ ##F; \
    } \
    int_unlock(lock); \
    return 0; \
}

BOOT_TEXT_SRAM_LOC SYS_SEL_PLL_FUNC(flash, FLASH);
SYS_SEL_PLL_FUNC(mem, MEM);
SYS_SEL_PLL_FUNC(sys, SYS);

int hal_cmu_get_sysclk(uint32_t *sysclk)
{
    *sysclk = cmu->SYS_CLK;
    return 0;
}

int hal_cmu_set_sysclk(uint32_t sysclk)
{
    cmu->SYS_CLK = sysclk;
    return 0;
}

int hal_cmu_pll_enable(enum HAL_CMU_PLL_T pll, enum HAL_CMU_PLL_USER_T user)
{
    uint32_t lock;
    uint32_t start;
    uint32_t check = 0;

    if (pll >= HAL_CMU_PLL_QTY) {
        return 1;
    }
    if (user >= HAL_CMU_PLL_USER_QTY) {
        return 2;
    }

    lock = int_lock();
    if (pll_user_map[pll] == 0 || user == HAL_CMU_PLL_USER_INIT) {
        if (pll == HAL_CMU_PLL_AUD) {
            cmu->SYS_CLK |= CMU_PU_PLL_AUD;
            cmu->SYS_CLK |= CMU_EN_PLL_AUD;
            check = CMU_LOCKED_PLL_AUD;
        } else {
            cmu->SYS_CLK |= CMU_PU_PLL_USB;
            cmu->SYS_CLK |= CMU_EN_PLL_USB;
            check = CMU_LOCKED_PLL_USB;
        }
    }
    if (user != HAL_CMU_PLL_USER_INIT) {
        pll_user_map[pll] |= (1 << user);
    }
    int_unlock(lock);

    if (check) {
        start = hal_sys_timer_get();
        while ((cmu->CLK_OUT & check) == 0 &&
                (hal_sys_timer_get() - start) < HAL_CMU_PLL_LOCKED_TIMEOUT);

        return (cmu->CLK_OUT & check) ? 0 : 2;
    } else {
        return 0;
    }
}

int hal_cmu_pll_disable(enum HAL_CMU_PLL_T pll, enum HAL_CMU_PLL_USER_T user)
{
    uint32_t lock;

    if (pll >= HAL_CMU_PLL_QTY) {
        return 1;
    }
    if (user >= HAL_CMU_PLL_USER_QTY) {
        return 2;
    }

    lock = int_lock();
    if (pll_user_map[pll] == (1 << user) || user == HAL_CMU_PLL_USER_INIT) {
        if (pll == HAL_CMU_PLL_AUD) {
            cmu->SYS_CLK &= ~CMU_EN_PLL_AUD;
            cmu->SYS_CLK &= ~CMU_PU_PLL_AUD;
        } else {
            cmu->SYS_CLK &= ~CMU_EN_PLL_USB;
            cmu->SYS_CLK &= ~CMU_PU_PLL_USB;
        }
    }
    if (user != HAL_CMU_PLL_USER_INIT) {
        pll_user_map[pll] &= ~(1 << user);
    }
    int_unlock(lock);

    return 0;
}

int hal_cmu_sys_get_pll_status(enum HAL_CMU_PLL_T *pll, uint32_t *status)
{
    uint32_t lock;

    lock = int_lock();

    if (cmu->SYS_CLK & CMU_SEL_PLLUSB_SYS){
        *pll = HAL_CMU_PLL_USB;
        if (cmu->SYS_CLK & (CMU_EN_PLL_USB|CMU_PU_PLL_USB))
            *status = true;
        else
            *status = false;
    }else{
        *pll = HAL_CMU_PLL_AUD;
        if (cmu->SYS_CLK & (CMU_EN_PLL_AUD|CMU_PU_PLL_AUD))
            *status = true;
        else
            *status = false;
    }

    int_unlock(lock);
    return 0;
}

#ifdef __AUDIO_RESAMPLE__
void hal_cmu_audio_resample_enable(void)
{
    aud_resample_en = 1;
}

void hal_cmu_audio_resample_disable(void)
{
    aud_resample_en = 0;
}

int hal_cmu_get_audio_resample_status(void)
{
    return aud_resample_en;
}
#endif

int hal_cmu_codec_adc_set_freq(enum HAL_CMU_CODEC_FREQ_T freq)
{
#if defined(__AUDIO_DIV_10___)
    static const uint32_t div[HAL_CMU_CODEC_FREQ_QTY] = { 10, 8 };
#else
	static const uint32_t div[HAL_CMU_CODEC_FREQ_QTY] = { 10, 7 };
#endif
    uint32_t lock;

    if (freq >= HAL_CMU_CODEC_FREQ_QTY) {
        return 1;
    }

    lock = int_lock();
    cmu->AUD_CLK = SET_BITFIELD(cmu->AUD_CLK, CMU_CFG_DIV_CODEC_ADC, div[freq])|CMU_POL_CLK_CODEC_ADC_IN;
    int_unlock(lock);
    return 0;
}

int hal_cmu_codec_dac_set_freq(enum HAL_CMU_CODEC_FREQ_T freq)
{
#if defined(__AUDIO_DIV_10___)
		static const uint32_t div[HAL_CMU_CODEC_FREQ_QTY] = { 10, 8 };
#else
		static const uint32_t div[HAL_CMU_CODEC_FREQ_QTY] = { 10, 7 };
#endif
    uint32_t lock;

    if (freq >= HAL_CMU_CODEC_FREQ_QTY) {
        return 1;
    }

    lock = int_lock();
#ifdef __AUDIO_RESAMPLE__
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
    {
        if (aud_resample_en) {
            cmu->SYS_CLK |= CMU_EN_PLL_AUD;
            cmu->AUD_CLK = SET_BITFIELD(cmu->AUD_CLK, CMU_CFG_DIV_CODEC_DAC, 0);
            cmu->MISC |= (1 << 5);
        } else {
            cmu->AUD_CLK = SET_BITFIELD(cmu->AUD_CLK, CMU_CFG_DIV_CODEC_DAC, div[freq]);
            cmu->MISC &= ~(1 << 5);
        }
    }
    else
#endif
    {
        cmu->AUD_CLK = SET_BITFIELD(cmu->AUD_CLK, CMU_CFG_DIV_CODEC_DAC, div[freq]);
    }

    int_unlock(lock);
    return 0;
}

uint32_t hal_cmu_codec_adc_get_div(void)
{
    return GET_BITFIELD(cmu->AUD_CLK, CMU_CFG_DIV_CODEC_ADC) + 2;
}

uint32_t hal_cmu_codec_dac_get_div(void)
{
    return GET_BITFIELD(cmu->AUD_CLK, CMU_CFG_DIV_CODEC_DAC) + 2;
}

void hal_cmu_anc_enable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->AUD_CLK |= CMU_EN_CLK_CODEC_ANC;
    int_unlock(lock);
}

void hal_cmu_anc_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->AUD_CLK &= ~CMU_EN_CLK_CODEC_ANC;
    int_unlock(lock);
}

int hal_cmu_anc_get_status(void)
{
    return !!(cmu->AUD_CLK & CMU_EN_CLK_CODEC_ANC);
}

void hal_cmu_codec_clock_enable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->AUD_CLK |= CMU_EN_CODEC_CLK_SRC;
    int_unlock(lock);
}

void hal_cmu_codec_clock_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->AUD_CLK &= ~CMU_EN_CODEC_CLK_SRC;
    int_unlock(lock);
}

void hal_cmu_i2s_clock_enable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->AUD_CLK |= CMU_EN_CLK_I2S_OUT;
    int_unlock(lock);
}

void hal_cmu_i2s_clock_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->AUD_CLK &= ~CMU_EN_CLK_I2S_OUT;
    int_unlock(lock);
}

void hal_cmu_i2s_set_slave_mode(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->AUD_CLK |= CMU_SEL_I2S_CLKIN;
    int_unlock(lock);
}

void hal_cmu_i2s_set_master_mode(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->AUD_CLK &= ~CMU_SEL_I2S_CLKIN;
    int_unlock(lock);
}

int hal_cmu_i2s_set_div(uint32_t div)
{
    uint32_t lock;

    if (div < 2) {
        return 1;
    }

    div -= 2;
    if ((div & (CMU_CFG_DIV_I2S_MASK >> CMU_CFG_DIV_I2S_SHIFT)) != div) {
        return 1;
    }

    lock = int_lock();
    cmu->AUD_CLK = SET_BITFIELD(cmu->AUD_CLK, CMU_CFG_DIV_I2S, div);
    int_unlock(lock);
    return 0;
}

int hal_cmu_pcm_set_div(uint32_t div)
{
    uint32_t lock;

    if (div < 2) {
        return 1;
    }

    div -= 2;
    if ((div & (CMU_CFG_DIV_PCM_MASK >> CMU_CFG_DIV_PCM_SHIFT)) != div) {
        return 1;
    }

    lock = int_lock();
    cmu->USB_CLK = SET_BITFIELD(cmu->USB_CLK, CMU_CFG_DIV_PCM, div);
    int_unlock(lock);
    return 0;
}

void hal_cmu_usb_set_device_mode(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->USB_CLK |= CMU_USB_ID;
    int_unlock(lock);
}

void hal_cmu_usb_set_host_mode(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->USB_CLK &= ~CMU_USB_ID;
    int_unlock(lock);
}

static void hal_cmu_usb_init_div(void)
{
    cmu->USB_CLK = (cmu->USB_CLK & ~(CMU_CFG_DIV_USB60M_MASK | CMU_CFG_DIV_USB48M_MASK)) |
        CMU_CFG_DIV_USB60M(HAL_CMU_USB_PLL_CLOCK / HAL_CMU_USB_CLOCK_60M - 2) |
        CMU_CFG_DIV_USB48M(HAL_CMU_USB_PLL_CLOCK / HAL_CMU_USB_CLOCK_48M - 2);
}

static void hal_cmu_apb_init_div(void)
{
    // Divider defaults to 2 (reg_val = div - 2)
    cmu->USB_CLK = SET_BITFIELD(cmu->USB_CLK, CMU_CFG_DIV_PSYS, 0);
}

#define PERPH_SET_FREQ_FUNC(f, F) \
int hal_cmu_ ##f## _set_freq(enum HAL_CMU_PERIPH_FREQ_T freq) \
{ \
    uint32_t lock; \
    int ret = 0; \
    lock = int_lock(); \
    if (freq == HAL_CMU_PERIPH_FREQ_26M) { \
        cmu->CLK_OUT |= CMU_SEL_ ##F## _26M; \
    } else if (freq == HAL_CMU_PERIPH_FREQ_52M) { \
        cmu->CLK_OUT &= ~CMU_SEL_ ##F## _26M; \
        cmu->PERIPH_CLK &= ~CMU_SEL_ ##F## _PLL; \
    } else { \
        ret = 1; \
    } \
    int_unlock(lock); \
    return ret; \
}

PERPH_SET_FREQ_FUNC(uart0, UART0);
PERPH_SET_FREQ_FUNC(uart1, UART1);
PERPH_SET_FREQ_FUNC(spi, SPI);
PERPH_SET_FREQ_FUNC(slcd, SLCD);
PERPH_SET_FREQ_FUNC(sdio, SDIO);
PERPH_SET_FREQ_FUNC(sdmmc, SDMMC);

int hal_cmu_i2c_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq)
{
    uint32_t lock;
    int ret = 0;

    lock = int_lock();
    if (freq == HAL_CMU_PERIPH_FREQ_26M) {
        cmu->CLK_OUT |= CMU_SEL_I2C_26M;
    } else if (freq == HAL_CMU_PERIPH_FREQ_52M) {
        cmu->CLK_OUT &= ~CMU_SEL_I2C_26M;
        cmu->USB_CLK &= ~CMU_SEL_I2C_PLL;
    } else {
        ret = 1;
    }
    int_unlock(lock);

    return ret;
}

int hal_cmu_ispi_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq)
{
    uint32_t lock;
    int ret = 0;

    lock = int_lock();
    if (freq == HAL_CMU_PERIPH_FREQ_26M) {
        cmu->CLK_OUT |= CMU_SEL_SPI_ITN_26M;
    } else if (freq == HAL_CMU_PERIPH_FREQ_52M) {
        cmu->CLK_OUT &= ~CMU_SEL_SPI_ITN_26M;
    } else {
        ret = 1;
    }
    int_unlock(lock);

    return ret;
}

int hal_cmu_clock_out_enable(enum HAL_CMU_CLOCK_OUT_ID_T id)
{
    uint32_t lock;

    if (id >= HAL_CMU_CLOCK_OUT_QTY) {
        return 1;
    }

    lock = int_lock();
    cmu->CLK_OUT = SET_BITFIELD(cmu->CLK_OUT, CMU_CFG_CLK_OUT, id) | CMU_CFG_CLK_OUT_EN;
    int_unlock(lock);

    return 0;
}

void hal_cmu_clock_out_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->CLK_OUT &= ~CMU_CFG_CLK_OUT_EN;
    int_unlock(lock);
}

int hal_cmu_pwm_set_freq(enum HAL_CMU_PWM_ID_T id, uint32_t freq)
{
    uint32_t lock;
    int clk_32k;
    uint32_t div;

    if (id >= HAL_CMU_PWM_ID_QTY) {
        return 1;
    }

    if (freq == HAL_CMU_PWM_SLOW_CLOCK) {
        clk_32k = 1;
        div = 0;
    } else {
        clk_32k = 0;
        div = HAL_CMU_PWM_FAST_CLOCK / freq;
        if (div < 2) {
            return 1;
        }

        div -= 2;
        if ((div & (CMU_CFG_DIV_PWM0_MASK >> CMU_CFG_DIV_PWM0_SHIFT)) != div) {
            return 1;
        }
    }

    lock = int_lock();
    if (id == HAL_CMU_PWM_ID_0) {
        cmu->PWM01_CLK = (cmu->PWM01_CLK & ~(CMU_CFG_DIV_PWM0_MASK | CMU_SEL_DIV_PWM0_32K)) |
            CMU_CFG_DIV_PWM0(div) | (clk_32k ? CMU_SEL_DIV_PWM0_32K : 0);
    } else if (id == HAL_CMU_PWM_ID_1) {
        cmu->PWM01_CLK = (cmu->PWM01_CLK & ~(CMU_CFG_DIV_PWM1_MASK | CMU_SEL_DIV_PWM1_32K)) |
            CMU_CFG_DIV_PWM1(div) | (clk_32k ? CMU_SEL_DIV_PWM1_32K : 0);
    } else if (id == HAL_CMU_PWM_ID_2) {
        cmu->PWM23_CLK = (cmu->PWM23_CLK & ~(CMU_CFG_DIV_PWM2_MASK | CMU_SEL_DIV_PWM2_32K)) |
            CMU_CFG_DIV_PWM2(div) | (clk_32k ? CMU_SEL_DIV_PWM2_32K : 0);
    } else {
        cmu->PWM23_CLK = (cmu->PWM23_CLK & ~(CMU_CFG_DIV_PWM3_MASK | CMU_SEL_DIV_PWM3_32K)) |
            CMU_CFG_DIV_PWM3(div) | (clk_32k ? CMU_SEL_DIV_PWM3_32K : 0);
    }
    int_unlock(lock);
    return 0;
}

void hal_cmu_write_lock(void)
{
    cmu->WRITE_UNLOCK = 0xCAFE0000;
}

void hal_cmu_write_unlock(void)
{
    cmu->WRITE_UNLOCK = 0xCAFE0001;
}

void hal_cmu_sys_reboot(void)
{
    hal_cmu_reset_set(HAL_CMU_MOD_P_GLOBAL);
}

void hal_cmu_jtag_enable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->CLK_OUT &= ~CMU_DISABLE_JTAG_I2C;
    int_unlock(lock);
}

void hal_cmu_jtag_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->CLK_OUT |= CMU_DISABLE_JTAG_I2C;
    int_unlock(lock);
}

void hal_cmu_simu_init(void)
{
    cmu->SIMU_RES = 0;
}

void hal_cmu_simu_pass(void)
{
    cmu->SIMU_RES = CMU_SIMU_RES_PASSED;
}

void hal_cmu_simu_fail(void)
{
    cmu->SIMU_RES = CMU_SIMU_RES_FAILED;
}

void hal_cmu_simu_tag(uint8_t shift)
{
    cmu->SIMU_RES |= (1 << shift);
}

void hal_cmu_simu_setval(uint32_t val)
{
    cmu->SIMU_RES = val;
}

void hal_cmu_low_freq_mode_enable(void)
{
    hal_cmu_pll_enable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SWITCH);
    hal_cmu_pll_enable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_SWITCH);
    // Clock mux requirs that both clocks are available before changing switch
    hal_cmu_sys_select_pll(HAL_CMU_PLL_USB);
    hal_cmu_pll_disable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SWITCH);
    hal_cmu_pll_disable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_SWITCH);
    // SYS will NOT use AUD PLL from now on
    hal_cmu_pll_disable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SYS);
}

void hal_cmu_low_freq_mode_disable(void)
{
    hal_cmu_pll_enable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SYS);
    // SYS will use AUD PLL later
    hal_cmu_pll_enable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SWITCH);
    hal_cmu_pll_enable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_SWITCH);
    // Clock mux requirs that both clocks are available before changing switch
    hal_cmu_sys_select_pll(HAL_CMU_PLL_AUD);
    hal_cmu_pll_disable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SWITCH);
    hal_cmu_pll_disable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_SWITCH);
}

static void hal_cmu_wait_26m_ready(void)
{
    while ((cmu->WAKEUP_CLK_CFG & CMU_LPU_26M_READY) == 0);
}

void hal_cmu_rom_setup(void)
{
    hal_cmu_simu_init();
    hal_cmu_timer_select_slow();
    hal_cmu_wait_26m_ready();
    // Init system clock to 78M on USB PLL (USB requires system clock to be 52M or above)
    hal_cmu_pll_enable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_SYS);
    hal_cmu_sys_select_pll(HAL_CMU_PLL_USB);
    hal_cmu_sys_set_freq(HAL_CMU_FREQ_78M);
    // Init peripheral clocks
    hal_cmu_apb_init_div();
    hal_cmu_usb_init_div();
    hal_cmu_ispi_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_flash_set_freq(HAL_CMU_FREQ_26M);

    hal_cmu_uart0_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_uart1_set_freq(HAL_CMU_PERIPH_FREQ_26M);
}

void hal_cmu_fpga_setup(void)
{
    hal_cmu_timer_select_slow();
    hal_cmu_sys_set_freq(HAL_CMU_FREQ_52M);
    hal_cmu_apb_init_div();
    hal_cmu_usb_init_div();
    hal_cmu_ispi_set_freq(HAL_CMU_PERIPH_FREQ_26M);

    hal_cmu_flash_set_freq(HAL_CMU_FREQ_26M);
    hal_norflash_init();
    hal_cmu_mem_set_freq(HAL_CMU_FREQ_52M);
    // TODO: Init psram

    hal_cmu_uart0_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_uart1_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_spi_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_slcd_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_sdio_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_sdmmc_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_i2c_set_freq(HAL_CMU_PERIPH_FREQ_26M);
}

static void BOOT_TEXT_FLASH_LOC hal_cmu_init_ema(void)
{
    uint32_t ema;

    ema = cmu->BOOT_DVS;
    ema = (ema & ~(CMU_ROM_EMA_MASK | CMU_RAM_EMA_MASK)) |
        CMU_ROM_EMA(0) | CMU_RAM_EMA(0);
    cmu->BOOT_DVS = ema;
}

void BOOT_TEXT_FLASH_LOC hal_cmu_setup(void)
{
    int ret;

    cmu->SYS_CLK |= CMU_BYPASS_LOCK_FLASH|CMU_BYPASS_LOCK_MEM|CMU_BYPASS_LOCK_SYS;

#ifndef SIMU
    cmu->ORESET_SET = CMU_ORST_PSRAM | CMU_ORST_USB | CMU_ORST_SDMMC | CMU_ORST_SDIO | CMU_ORST_SPI | CMU_ORST_SLCD |
        CMU_ORST_UART0 | CMU_ORST_UART1 | CMU_ORST_CODEC_AD | CMU_ORST_CODEC_DA | CMU_ORST_I2S | CMU_ORST_SPDIF | CMU_ORST_PCM |
        CMU_ORST_I2C | CMU_ORST_PWM0 | CMU_ORST_PWM1 | CMU_ORST_PWM2 | CMU_ORST_PWM3;
    cmu->PRESET_SET = CMU_PRST_PWM | CMU_PRST_I2C | CMU_PRST_SPI | CMU_PRST_SLCD | CMU_PRST_UART0 | CMU_PRST_UART1 |
        CMU_PRST_CODEC | CMU_PRST_PCM | CMU_PRST_I2S | CMU_PRST_SPDIF;
    cmu->HRESET_SET = CMU_HRST_PSRAM | CMU_HRST_SDIO | CMU_HRST_SDMMC | CMU_HRST_USBC | CMU_HRST_DPDRX | CMU_HRST_DPDTX;

    cmu->OCLK_DISABLE = CMU_OCLK_PSRAM | CMU_OCLK_USB | CMU_OCLK_SDMMC | CMU_OCLK_SDIO | CMU_OCLK_SPI | CMU_OCLK_SLCD |
        CMU_OCLK_UART0 | CMU_OCLK_UART1 | CMU_OCLK_I2S | CMU_OCLK_SPDIF | CMU_OCLK_PCM |
        CMU_OCLK_I2C | CMU_OCLK_PWM0 | CMU_OCLK_PWM1 | CMU_OCLK_PWM2 | CMU_OCLK_PWM3;
    cmu->PCLK_DISABLE = CMU_PCLK_PWM | CMU_PCLK_I2C | CMU_PCLK_SPI | CMU_PCLK_SLCD | CMU_PCLK_UART0 | CMU_PCLK_UART1 |
        CMU_PCLK_CODEC | CMU_PCLK_PCM | CMU_PCLK_I2S | CMU_PCLK_SPDIF;
    cmu->HCLK_DISABLE = CMU_HCLK_PSRAM | CMU_HCLK_SDIO | CMU_HCLK_SDMMC | CMU_HCLK_USBC | CMU_HCLK_DPDRX | CMU_HCLK_DPDTX;

    // Disable codec clock, and set I2S/SPDIF divider to max
    cmu->AUD_CLK = CMU_CFG_DIV_CODEC_ADC(0xF) | CMU_CFG_DIV_CODEC_DAC(0xF) | CMU_CFG_DIV_I2S(0x1FFF);
    cmu->USB_CLK = (cmu->USB_CLK & ~(CMU_CFG_DIV_I2C_MASK | CMU_CFG_DIV_PCM_MASK)) |
        CMU_CFG_DIV_I2C(0xFF) | CMU_CFG_DIV_PCM(0x1FFF);
#endif

    hal_cmu_init_ema();
    hal_cmu_timer_select_slow();
    hal_sys_timer_open();

    hal_cmu_pll_enable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SYS);
    // Init system/flash/memory clocks before switching PLL
    hal_cmu_sys_set_freq(HAL_CMU_FREQ_26M);
    hal_cmu_flash_set_freq(HAL_CMU_FREQ_26M);
    hal_cmu_mem_set_freq(HAL_CMU_FREQ_26M);

#ifdef ULTRA_LOW_POWER
    // Switch system/flash/memory clocks to USB PLL, and then shutdown USB PLL,
    // to save power consumed in clock dividers
    hal_cmu_sys_select_pll(HAL_CMU_PLL_USB);
    hal_cmu_flash_select_pll(HAL_CMU_PLL_USB);
    hal_cmu_mem_select_pll(HAL_CMU_PLL_USB);
#else
    // Switch system/flash/memory clocks to audio PLL
    hal_cmu_sys_select_pll(HAL_CMU_PLL_AUD);
    hal_cmu_flash_select_pll(HAL_CMU_PLL_AUD);
    hal_cmu_mem_select_pll(HAL_CMU_PLL_AUD);
#endif

    hal_cmu_ispi_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_uart0_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_uart1_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_spi_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_slcd_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_sdio_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_sdmmc_set_freq(HAL_CMU_PERIPH_FREQ_52M);
    hal_cmu_i2c_set_freq(HAL_CMU_PERIPH_FREQ_26M);

    // Disable USB PLL after switching (clock mux requirement)
    hal_cmu_pll_disable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_INIT);
#ifdef ULTRA_LOW_POWER
    hal_cmu_pll_disable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SYS);
#endif

    // Open analogif (ISPI) after setting ISPI module freq
    ret = hal_analogif_open();
    if (ret) {
        hal_cmu_simu_tag(31);
        while (1);
    }
    hal_chipid_init();

    // Sleep setting
#ifdef NO_LPU_26M
    while (hal_sleep_config_clock(HAL_SLEEP_LPU_CLK_NONE, 0x30, 0) == -1);
#else
    while (hal_sleep_config_clock(HAL_SLEEP_LPU_CLK_26M, 0x30, 0) == -1);
#endif
    // Init sys freq after applying the sleep setting (which might change sys freq)
#ifdef NO_LPU_26M
   hal_sys_timer_delay(MS_TO_TICKS(20));
#endif

    // Init system clock
    {
#ifdef ULTRA_LOW_POWER
        hal_sysfreq_req(HAL_SYSFREQ_USER_RESVALID_0, HAL_CMU_FREQ_52M);
#else
        hal_sysfreq_req(HAL_SYSFREQ_USER_RESVALID_0, HAL_CMU_FREQ_104M);
#endif
    }

    // Init APB divider
    hal_cmu_apb_init_div();
    hal_cmu_usb_init_div();

    // Init flash
    hal_norflash_init();

    // Init psram

    // TODO: Call psram init function and move mem freq setting into the function
    {
#ifdef ULTRA_LOW_POWER
        hal_cmu_mem_set_freq(HAL_CMU_FREQ_26M);
#else
        hal_cmu_mem_set_freq(HAL_CMU_FREQ_78M);
#endif
    }
    // TODO: Call psram init function and move mem freq setting into the function

    hal_cmu_uart0_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_uart1_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_spi_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_slcd_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_sdio_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_sdmmc_set_freq(HAL_CMU_PERIPH_FREQ_26M);
    hal_cmu_i2c_set_freq(HAL_CMU_PERIPH_FREQ_26M);
}

