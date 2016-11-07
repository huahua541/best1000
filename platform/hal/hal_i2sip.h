#ifndef __HAL_I2SIP_H__
#define __HAL_I2SIP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "reg_i2sip.h"

#define i2sip_read32(b,a) \
     (*(volatile uint32_t *)(b+a))

#define i2sip_write32(v,b,a) \
     ((*(volatile uint32_t *)(b+a)) = v)

static inline void i2sip_w_enable_i2sip(uint32_t reg_base, uint32_t v)
{
    uint32_t val = 0;

    val = i2sip_read32(reg_base, I2SIP_ENABLE_REG_REG_OFFSET);
    if (v)
        val |= I2SIP_ENABLE_REG_I2S_ENABLE_MASK;
    else
        val &= ~I2SIP_ENABLE_REG_I2S_ENABLE_MASK;

    i2sip_write32(val, reg_base, I2SIP_ENABLE_REG_REG_OFFSET);
}
static inline void i2sip_w_enable_clk_gen(uint32_t reg_base, uint32_t v)
{
    if (v)
        i2sip_write32(1, reg_base, I2SIP_CLK_GEN_ENABLE_REG_REG_OFFSET);
    else
        i2sip_write32(0, reg_base, I2SIP_CLK_GEN_ENABLE_REG_REG_OFFSET);
}
static inline void i2sip_w_enable_rx_block(uint32_t reg_base, uint32_t v)
{
    if (v)
        i2sip_write32(1, reg_base, I2SIP_RX_BLOCK_ENABLE_REG_REG_OFFSET);
    else
        i2sip_write32(0, reg_base, I2SIP_RX_BLOCK_ENABLE_REG_REG_OFFSET);
}
static inline void i2sip_w_enable_rx_channel0(uint32_t reg_base, uint32_t v)
{
    if (v)
        i2sip_write32(1, reg_base, I2SIP_RX_ENABLE_REG_OFFSET);
    else
        i2sip_write32(0, reg_base, I2SIP_RX_ENABLE_REG_OFFSET);
}
static inline void i2sip_w_enable_tx_block(uint32_t reg_base, uint32_t v)
{
    if (v)
        i2sip_write32(1, reg_base, I2SIP_TX_BLOCK_ENABLE_REG_REG_OFFSET);
    else
        i2sip_write32(0, reg_base, I2SIP_TX_BLOCK_ENABLE_REG_REG_OFFSET);
}
static inline void i2sip_w_enable_tx_channel0(uint32_t reg_base, uint32_t v)
{
    if (v)
        i2sip_write32(1, reg_base, I2SIP_TX_ENABLE_REG_OFFSET);
    else
        i2sip_write32(0, reg_base, I2SIP_TX_ENABLE_REG_OFFSET);
}
static inline void i2sip_w_tx_resolution(uint32_t reg_base, uint32_t v)
{
    i2sip_write32(v<<I2SIP_TX_CFG_WLEN_SHIFT, reg_base, I2SIP_TX_CFG_REG_OFFSET);
}
static inline void i2sip_w_rx_resolution(uint32_t reg_base, uint32_t v)
{
    i2sip_write32(v<<I2SIP_RX_CFG_WLEN_SHIFT, reg_base, I2SIP_RX_CFG_REG_OFFSET);
}
static inline void i2sip_w_clk_cfg_reg(uint32_t reg_base, uint32_t v)
{
    i2sip_write32(v, reg_base, I2SIP_CLK_CFG_REG_OFFSET);
}
static inline void i2sip_w_tx_left_fifo(uint32_t reg_base, uint32_t v)
{
    i2sip_write32(v, reg_base, I2SIP_LEFT_TX_BUFF_REG_OFFSET);
}
static inline void i2sip_w_tx_right_fifo(uint32_t reg_base, uint32_t v)
{
    i2sip_write32(v, reg_base, I2SIP_RIGHT_TX_BUFF_REG_OFFSET);
}
static inline void i2sip_w_tx_fifo_threshold(uint32_t reg_base, uint32_t v)
{
    i2sip_write32(v<<I2SIP_TX_FIFO_CFG_LEVEL_SHIFT, reg_base, I2SIP_TX_FIFO_CFG_REG_OFFSET);
}
static inline void i2sip_w_rx_fifo_threshold(uint32_t reg_base, uint32_t v)
{
    i2sip_write32(v<<I2SIP_RX_FIFO_CFG_LEVEL_SHIFT, reg_base, I2SIP_RX_FIFO_CFG_REG_OFFSET);
}
static inline uint32_t i2sip_r_int_status(uint32_t reg_base)
{
    return i2sip_read32(reg_base, I2SIP_INT_STATUS_REG_OFFSET);
}
static inline void i2sip_w_enable_tx_dma(uint32_t reg_base, uint32_t v)
{
    uint32_t val = 0;
    val = i2sip_read32(reg_base, I2SIP_DMA_CTRL_REG_OFFSET);
    if (v)
        val |= I2SIP_DMA_CTRL_TX_ENABLE_MASK;
    else
        val &= ~I2SIP_DMA_CTRL_TX_ENABLE_MASK;

    i2sip_write32(val, reg_base, I2SIP_DMA_CTRL_REG_OFFSET);
}
static inline void i2sip_w_enable_rx_dma(uint32_t reg_base, uint32_t v)
{
    uint32_t val = 0;
    val = i2sip_read32(reg_base, I2SIP_DMA_CTRL_REG_OFFSET);
    if (v)
        val |= I2SIP_DMA_CTRL_RX_ENABLE_MASK;
    else
        val &= ~I2SIP_DMA_CTRL_RX_ENABLE_MASK;

    i2sip_write32(val, reg_base, I2SIP_DMA_CTRL_REG_OFFSET);
}

#ifdef __cplusplus
}
#endif

#endif /* __HAL_I2SIP_H__ */
