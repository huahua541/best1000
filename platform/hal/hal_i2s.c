#include "plat_types.h"
#include "reg_i2sip.h"
#include "hal_i2sip.h"
#include "hal_i2s.h"
#include "hal_uart.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_iomux.h"
#include "analog.h"

#include "hal_cmu.h"

#define HAL_I2S_TX_FIFO_TRIGGER_LEVEL   (I2SIP_FIFO_DEPTH/2)
#define HAL_I2S_RX_FIFO_TRIGGER_LEVEL   (I2SIP_FIFO_DEPTH/2)

#define HAL_I2S_YES 1
#define HAL_I2S_NO 0

static const char * const invalid_id = "Invalid I2S ID: %d\n";
static const char * const invalid_ch = "Invalid I2S CH: %d\n";

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

static void hal_i2s_iomux()
{
#if FPGA == 0
    // use i2s3
    const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2s[] = {
        {HAL_IOMUX_PIN_P2_2, HAL_IOMUX_FUNC_I2S_WS, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P2_3, HAL_IOMUX_FUNC_I2S_SDI, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P2_4, HAL_IOMUX_FUNC_I2S_SDO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P2_5, HAL_IOMUX_FUNC_I2S_SCK, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };

    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_i2s, sizeof(pinmux_i2s)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
#else
    // use i2s1
    *((volatile uint32_t *)(0x4001f004)) |= 0x1<<12;
#endif
}


static inline uint32_t _i2s_get_reg_base(enum HAL_I2S_ID_T id)
{
    ASSERT(id < HAL_I2S_ID_NUM, invalid_id, id);
    switch(id) {
        case HAL_I2S_ID_0:
        default:
            return 0x4001B000;
            break;
    }
	return 0;
}

int hal_i2s_open(enum HAL_I2S_ID_T id)
{
    uint32_t reg_base;

    reg_base = _i2s_get_reg_base(id);
    
	hal_i2s_iomux();
    hal_cmu_clock_enable(HAL_CMU_MOD_O_I2S);
    hal_cmu_clock_enable(HAL_CMU_MOD_P_I2S);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_I2S);
    hal_cmu_reset_clear(HAL_CMU_MOD_P_I2S);
    i2sip_w_enable_i2sip(reg_base, HAL_I2S_YES);

    return 0;
}

int hal_i2s_close(enum HAL_I2S_ID_T id)
{
    uint32_t reg_base;
    
    reg_base = _i2s_get_reg_base(id);

    i2sip_w_enable_i2sip(reg_base, HAL_I2S_NO);
    hal_cmu_i2s_set_div(0x1FFF + 2);
    hal_cmu_reset_set(HAL_CMU_MOD_P_I2S);
    hal_cmu_reset_set(HAL_CMU_MOD_O_I2S);
    hal_cmu_clock_disable(HAL_CMU_MOD_P_I2S);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_I2S);

    return 0;
}

int hal_i2s_start_stream(enum HAL_I2S_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t reg_base;

    reg_base = _i2s_get_reg_base(id);

    if (stream == AUD_STREAM_PLAYBACK) 
    {
        i2sip_w_enable_tx_block(reg_base, HAL_I2S_YES);
        i2sip_w_enable_tx_channel0(reg_base, HAL_I2S_YES);

        //master mode
        hal_cmu_i2s_set_master_mode();
        hal_cmu_i2s_clock_enable();
        i2sip_w_enable_clk_gen(reg_base, HAL_I2S_YES);
    }
    else
    {
        i2sip_w_enable_rx_block(reg_base, HAL_I2S_YES);
        i2sip_w_enable_rx_channel0(reg_base, HAL_I2S_YES);

        //slave mode
        hal_cmu_i2s_set_slave_mode();
    }

    return 0;
}

int hal_i2s_stop_stream(enum HAL_I2S_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t reg_base;

    reg_base = _i2s_get_reg_base(id);

    if (stream == AUD_STREAM_PLAYBACK) 
    {
        // Master disable output CLK
        hal_cmu_i2s_clock_disable();
        i2sip_w_enable_clk_gen(reg_base, HAL_I2S_NO); 
        
        i2sip_w_enable_tx_block(reg_base, HAL_I2S_NO);
        i2sip_w_enable_tx_channel0(reg_base, HAL_I2S_NO);
    }
    else 
    {
        i2sip_w_enable_rx_block(reg_base, HAL_I2S_NO);
        i2sip_w_enable_rx_channel0(reg_base, HAL_I2S_NO);
    }

    return 0;
}


int hal_i2s_setup_stream(enum HAL_I2S_ID_T id, enum AUD_STREAM_T stream, struct HAL_I2S_CONFIG_T *cfg)
{
    uint32_t reg_base;
    uint32_t sclk = 0, div = 0;
    uint8_t resolution = 0, word_sclk = 0;

    reg_base = _i2s_get_reg_base(id);

    switch (cfg->bits) 
    {
        case AUD_BITS_8:
            word_sclk  = I2SIP_CLK_CFG_WSS_VAL_16CYCLE;
            resolution = I2SIP_CFG_WLEN_VAL_IGNORE;
            break;
        case AUD_BITS_12:
            word_sclk  = I2SIP_CLK_CFG_WSS_VAL_16CYCLE;
            resolution = I2SIP_CFG_WLEN_VAL_12BIT;
            break;
        case AUD_BITS_16:
            word_sclk  = I2SIP_CLK_CFG_WSS_VAL_16CYCLE;
            resolution = I2SIP_CFG_WLEN_VAL_16BIT;
            break;
        case AUD_BITS_20:
            word_sclk  = I2SIP_CLK_CFG_WSS_VAL_16CYCLE;
            resolution = I2SIP_CFG_WLEN_VAL_20BIT;
            break;
        case AUD_BITS_24:
            word_sclk  = I2SIP_CLK_CFG_WSS_VAL_24CYCLE;
            resolution = I2SIP_CFG_WLEN_VAL_24BIT;
            break;
        case AUD_BITS_32:
            word_sclk  = I2SIP_CLK_CFG_WSS_VAL_32CYCLE;
            resolution = I2SIP_CFG_WLEN_VAL_32BIT;
            break;
        default:
            return 0;
    }

    /* word clock width : how many sclk work sclk count*/
    /* sclk gate in word clock : how many sclk gate : fixed no gate */
    i2sip_w_clk_cfg_reg(reg_base, (word_sclk<<I2SIP_CLK_CFG_WSS_SHIFT) | (I2SIP_CLK_CFG_SCLK_GATE_VAL_NO_GATE<<I2SIP_CLK_CFG_SCLK_GATE_SHIFT));

#if FPGA==0
	// TODO: More sample rate, channel number and bits
	if(cfg->sample_rate == AUD_SAMPRATE_44100)
	{
		//set audio pll
       	//203212.8k
       	analog_aud_freq_pll_config(HAL_CMU_CODEC_FREQ_44_1K,AUD_SAMPRATE_44100);
	}
	else if(cfg->sample_rate == AUD_SAMPRATE_48000)
	{
		//set audio pll
		//221184k
		analog_aud_freq_pll_config(HAL_CMU_CODEC_FREQ_48K,AUD_SAMPRATE_48000);
	}
    else
    {
        ASSERT(0, "[%s] Do not support this sample rate = %d", __func__, cfg->sample_rate);
    }
	// FIXME: Channel num = 2, bits = 32
	div  = 16*9;
	
	hal_cmu_i2s_set_div(div);

	hal_cmu_reset_pulse(HAL_CMU_MOD_O_I2S);
#else
	sclk = cfg->sample_rate * cfg->bits * cfg->channel_num;

#define I2S_CLOCK_SOURCE 22579200	//44100*512
    div  = I2S_CLOCK_SOURCE/sclk - 1;
#undef I2S_CLOCK_SOURCE

    TRACE("div = %x",div);
    *((uint32_t *)0x40000064) = div<<15;
#endif

    if (stream == AUD_STREAM_PLAYBACK)
    {
        /* resolution : valid bit width */
        i2sip_w_tx_resolution(reg_base, resolution);

        /* fifo level to trigger empty interrupt or dma req */
        i2sip_w_tx_fifo_threshold(reg_base, HAL_I2S_TX_FIFO_TRIGGER_LEVEL);

        /* dma config */
        if (cfg->use_dma) 
        {
            i2sip_w_enable_tx_dma(reg_base, HAL_I2S_YES);
        }
        else
        {
            i2sip_w_enable_tx_dma(reg_base, HAL_I2S_NO);
        }
    }
    else
    {
        /* resolution : valid bit width */
        i2sip_w_rx_resolution(reg_base, resolution);

        /* fifo level to trigger empty interrupt or dma req */
        i2sip_w_rx_fifo_threshold(reg_base, HAL_I2S_RX_FIFO_TRIGGER_LEVEL);

        /* dma config */
        if(cfg->use_dma) 
        {
            i2sip_w_enable_rx_dma(reg_base, HAL_I2S_YES);
        }
        else
        {
            i2sip_w_enable_rx_dma(reg_base, HAL_I2S_NO);
        }
    }
    
    return 0;
}

int hal_i2s_send(enum HAL_I2S_ID_T id, uint8_t *value, uint32_t value_len)
{
    uint32_t i = 0;
    uint32_t reg_base;

    reg_base = _i2s_get_reg_base(id);

    for (i = 0; i < value_len; i += 4) {
        while (!(i2sip_r_int_status(reg_base) & I2SIP_INT_STATUS_TX_FIFO_EMPTY_MASK));

        i2sip_w_tx_left_fifo(reg_base, value[i+1]<<8 | value[i]);
        i2sip_w_tx_right_fifo(reg_base, value[i+3]<<8 | value[i+2]);
    }

    return 0;
}
uint8_t hal_i2s_recv(enum HAL_I2S_ID_T id, uint8_t *value, uint32_t value_len)
{
    uint32_t reg_base;
    
    reg_base = _i2s_get_reg_base(id);
    
    return 0;
}
