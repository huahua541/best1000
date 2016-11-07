#include "plat_types.h"
#include "reg_spdifip.h"
#include "hal_spdifip.h"
#include "hal_spdif.h"
#include "hal_uart.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_cmu.h"
#include "hal_iomux.h"
#include "analog.h"

//#define SPDIF_CLOCK_SOURCE 240000000
//#define SPDIF_CLOCK_SOURCE 22579200
//#define SPDIF_CLOCK_SOURCE 48000000
#define SPDIF_CLOCK_SOURCE 3072000

//#define SPDIF_CLOCK_SOURCE 76800000
//#define SPDIF_CLOCK_SOURCE 84672000

#define HAL_SPDIF_TX_FIFO_TRIGGER_LEVEL   (SPDIFIP_FIFO_DEPTH/2)
#define HAL_SPDIF_RX_FIFO_TRIGGER_LEVEL   (SPDIFIP_FIFO_DEPTH/2)

#define HAL_SPDIF_YES 1
#define HAL_SPDIF_NO 0

static const char * const invalid_id = "Invalid SPDIF ID: %d\n";
static const char * const invalid_ch = "Invalid SPDIF CH: %d\n";

//---------------------------------------------------------------
/*
#define I2S_BASE  0x40020000
#define I2S_DMA_BASE 0x400201c0

#define I2STXSRCBASEADDR  0x20000040
#define I2SRXDESTBASEADDR 0x200000C0
#define I2S_TXSIZE 8

typedef struct i2sregisters {
	volatile uint32_t IER;
	volatile uint32_t IRER;
	volatile uint32_t ITER;
	volatile uint32_t CER;
	volatile uint32_t CCR;      // 0x10
	volatile uint32_t RXFFR;
	volatile uint32_t TXFFR;
    volatile uint32_t rev0;
	volatile uint32_t LRTBR0;   // 0x20
	volatile uint32_t RRTBR0;
    volatile uint32_t RER0;
	volatile uint32_t TER0;
	volatile uint32_t RCR0;     // 0x30
	volatile uint32_t TCR0;
	volatile uint32_t ISR0;
	volatile uint32_t IMR0;
	volatile uint32_t ROR0;     // 0x40
	volatile uint32_t TOR0;
    volatile uint32_t RFCR0;
	volatile uint32_t TFCR0;
	volatile uint32_t RFF0;     // 0x50
	volatile uint32_t TFF0;
    volatile uint32_t DMACTRL0;
    volatile uint32_t rev2;
    volatile uint32_t SPDIFRXCFG;  // 0x60
    volatile uint32_t SPDIFRXSTAT;
    volatile uint32_t SPDIFRXUSRCHSTCFG;
    volatile uint32_t SPDIFRXUSRCHSTDATA;
    volatile uint32_t rev3[2];
    volatile uint32_t SPDIFTXCFG;
    volatile uint32_t rev4;
    volatile uint32_t SPDIFUSRCHSTDATA[24];
    volatile uint32_t SPDIFINTMASK;  // 0xe0
    volatile uint32_t SPDIFINTSTAT;
} i2sreg;

typedef struct i2sdmaregisters {
  volatile uint32_t RXDMA;
  volatile uint32_t RRXDMA;
  volatile uint32_t TXDMA;
  volatile uint32_t RTXDMA;
} i2sdmareg;

  uint32_t    *i2stxsrcaddr   = (uint32_t *)    I2STXSRCBASEADDR;
  uint32_t    *i2srxdestaddr  = (uint32_t *)    I2SRXDESTBASEADDR;
  i2sreg    *i2sregs        = (i2sreg *)    I2S_BASE;
  i2sdmareg *i2sdmaregs     = (i2sdmareg *) I2S_DMA_BASE;

*/
//---------------------------------------------------------------
static inline unsigned char reverse(unsigned char in)
{
    uint8_t out = 0;
    uint32_t i = 0;
    for (i = 0; i < 8; ++i) {
        if((1<<i & in))
            out = out | (1<<(7-i));
        else
            out = out & (~(1<<(7-i)));
    }

    return out;
}

static inline uint32_t _spdif_get_reg_base(enum HAL_SPDIF_ID_T id)
{
    switch(id) {
        case HAL_SPDIF_ID_0:
        default:
            return 0x40020000;
            break;
    }
	return 0;
}

static void hal_spdif_iomux()
{
    const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_spdif[] = {
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_SPDIF_DI, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_SPDIF_DO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };

    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_spdif, sizeof(pinmux_spdif)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
}

int hal_spdif_open(enum HAL_SPDIF_ID_T id)
{
    uint32_t reg_base;
    ASSERT(id < HAL_SPDIF_ID_NUM, invalid_id, id);

    // FIXME : iomux
    hal_spdif_iomux();

    hal_cmu_clock_enable(HAL_CMU_MOD_O_SPDIF0);
    hal_cmu_clock_enable(HAL_CMU_MOD_P_SPDIF0);
	hal_cmu_reset_clear(HAL_CMU_MOD_O_SPDIF0);
	hal_cmu_reset_clear(HAL_CMU_MOD_P_SPDIF0);

    reg_base = _spdif_get_reg_base(id);

//    i2sdmaregs->RRXDMA = 0x1;   // reset dma rx register.
//    i2sdmaregs->RTXDMA = 0x1;   // reset dma tx register.

    spdifip_w_enable_spdifip(reg_base, HAL_SPDIF_YES);

//    i2sregs->SPDIFRXUSRCHSTCFG = 0x20;

    return 0;
}

int hal_spdif_close(enum HAL_SPDIF_ID_T id)
{
    uint32_t reg_base;
    ASSERT(id < HAL_SPDIF_ID_NUM, invalid_id, id);
    reg_base = _spdif_get_reg_base(id);

    spdifip_w_enable_spdifip(reg_base, HAL_SPDIF_NO);

    hal_cmu_pcm_set_div(0x1FFF + 2);
	hal_cmu_reset_set(HAL_CMU_MOD_P_SPDIF0);
	hal_cmu_reset_set(HAL_CMU_MOD_O_SPDIF0);
    hal_cmu_clock_disable(HAL_CMU_MOD_P_SPDIF0);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_SPDIF0);

    return 0;
}

int hal_spdif_start_stream(enum HAL_SPDIF_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t reg_base;
    ASSERT(id < HAL_SPDIF_ID_NUM, invalid_id, id);
    reg_base = _spdif_get_reg_base(id);

    if (stream == AUD_STREAM_PLAYBACK) {
        spdifip_w_enable_tx_channel0(reg_base, HAL_SPDIF_YES);
        spdifip_w_enable_tx(reg_base, HAL_SPDIF_YES);
        spdifip_w_tx_valid(reg_base, HAL_SPDIF_YES);        //send data is valid

    }
    else {
        spdifip_w_enable_rx_channel0(reg_base, HAL_SPDIF_YES);
        spdifip_w_enable_rx(reg_base, HAL_SPDIF_YES);
        spdifip_w_sample_en(reg_base, HAL_SPDIF_YES);
    }

    return 0;
}

int hal_spdif_stop_stream(enum HAL_SPDIF_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t reg_base;
    ASSERT(id < HAL_SPDIF_ID_NUM, invalid_id, id);
    reg_base = _spdif_get_reg_base(id);

    if (stream == AUD_STREAM_PLAYBACK) {
        spdifip_w_enable_tx(reg_base, HAL_SPDIF_NO);
        spdifip_w_enable_tx_channel0(reg_base, HAL_SPDIF_NO);
    }
    else {
        spdifip_w_enable_rx(reg_base, HAL_SPDIF_NO);
        spdifip_w_enable_rx_channel0(reg_base, HAL_SPDIF_NO);
    }

    return 0;
}

int hal_spdif_setup_stream(enum HAL_SPDIF_ID_T id, enum AUD_STREAM_T stream, struct HAL_SPDIF_CONFIG_T *cfg)
{
    uint32_t reg_base;

    ASSERT(id < HAL_SPDIF_ID_NUM, invalid_id, id);

    reg_base = _spdif_get_reg_base(id);

//        i2sregs->SPDIFTXCFG |= 0x0000a3;

    // TODO: midify struct, like hal_codec
    if (stream == AUD_STREAM_PLAYBACK)
    {
        // TODO: need support more sample rate
        if(cfg->sample_rate == AUD_SAMPRATE_48000)
        {
#ifdef FPGA
            hal_cmu_pcm_set_div(2);
#else
            //set audio pll
            //221184k/(48k * 128)-->div = 36
            analog_aud_freq_pll_config(HAL_CMU_CODEC_FREQ_48K,AUD_SAMPRATE_48000);

            hal_cmu_pcm_set_div(6);
#endif
            spdifip_w_tx_ratio(reg_base, 5);
        }
        else if(cfg->sample_rate == AUD_SAMPRATE_44100)
        {
#ifdef FPGA
            hal_cmu_pcm_set_div(2);
#else
            //set audio pll
            //203212.8k/(44.1k * 128)-->div = 36
            analog_aud_freq_pll_config(HAL_CMU_CODEC_FREQ_44_1K,AUD_SAMPRATE_44100);

            hal_cmu_pcm_set_div(6);
#endif
            spdifip_w_tx_ratio(reg_base, 5);
        }
        else
        {
            ASSERT(0, "[%s] sample rate(%d) is not supported!!!",  __func__, cfg->sample_rate);
        }


        hal_cmu_reset_pulse(HAL_CMU_MOD_O_SPDIF0);


        if (cfg->use_dma)
        {
            spdifip_w_tx_fifo_threshold(reg_base, HAL_SPDIF_TX_FIFO_TRIGGER_LEVEL);
            spdifip_w_enable_tx_dma(reg_base, HAL_SPDIF_YES);
        }
    }
    else
    {
        if(cfg->sample_rate == AUD_SAMPRATE_48000)
        {
#ifdef FPGA
            hal_cmu_pcm_set_div(2);
#else
            //set audio pll
            //221184k/(48k * 128)-->div = 36
            analog_aud_freq_pll_config(HAL_CMU_CODEC_FREQ_48K,AUD_SAMPRATE_48000);

            hal_cmu_pcm_set_div(6);
#endif
            spdifip_w_tx_ratio(reg_base, 5);
        }
        else if(cfg->sample_rate == AUD_SAMPRATE_44100)
        {
#ifdef FPGA
            hal_cmu_pcm_set_div(2);
#else
            //set audio pll
            //203212.8k/(44.1k * 128)-->div = 36
            analog_aud_freq_pll_config(HAL_CMU_CODEC_FREQ_44_1K,AUD_SAMPRATE_44100);

            hal_cmu_pcm_set_div(6);
#endif
            spdifip_w_tx_ratio(reg_base, 5);
        }
        else
        {
            ASSERT(0, "[%s] sample rate(%d) is not supported!!!",  __func__, cfg->sample_rate);
        }


        hal_cmu_reset_pulse(HAL_CMU_MOD_O_SPDIF0);

        if (cfg->use_dma)
        {
            spdifip_w_rx_fifo_threshold(reg_base, HAL_SPDIF_RX_FIFO_TRIGGER_LEVEL);
            spdifip_w_enable_rx_dma(reg_base, HAL_SPDIF_YES);
        }
    }

    return 0;
}

int hal_spdif_send(enum HAL_SPDIF_ID_T id, uint8_t *value, uint32_t value_len)
{
    uint32_t i = 0;
    uint32_t reg_base;
    ASSERT(id < HAL_SPDIF_ID_NUM, invalid_id, id);

    reg_base = _spdif_get_reg_base(id);

    for (i = 0; i < value_len; i += 4) {
        while (!(spdifip_r_int_status(reg_base) & SPDIFIP_INT_STATUS_TX_FIFO_EMPTY_MASK));

        spdifip_w_tx_left_fifo(reg_base, value[i+1]<<8 | value[i]);
        spdifip_w_tx_right_fifo(reg_base, value[i+3]<<8 | value[i+2]);
    }

    return 0;
}

uint8_t hal_spdif_recv(enum HAL_SPDIF_ID_T id, uint8_t *value, uint32_t value_len)
{
    ASSERT(id < HAL_SPDIF_ID_NUM, invalid_id, id);
    return 0;
}
