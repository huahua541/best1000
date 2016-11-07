#ifndef __HAL_DMA_COMMON_H__
#define __HAL_DMA_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"

#define HAL_DMA_CHAN_NONE        0xFF

enum HAL_DMA_RET_T {
    HAL_DMA_OK,
    HAL_DMA_ERR,
};

enum HAL_DMA_GET_CHAN_T {
    HAL_DMA_HIGH_PRIO,
    HAL_DMA_LOW_PRIO,
    HAL_DMA_LOW_PRIO_ONLY
};

/**
 * DMA Type of DMA controller
 */
enum HAL_DMA_FLOW_CONTROL_T {
    HAL_DMA_FLOW_M2M_DMA              = 0,    /* Memory to memory - DMA control */
    HAL_DMA_FLOW_M2P_DMA              = 1,    /* Memory to peripheral - DMA control */
    HAL_DMA_FLOW_P2M_DMA              = 2,    /* Peripheral to memory - DMA control */
    HAL_DMA_FLOW_P2P_DMA              = 3,    /* Source peripheral to destination peripheral - DMA control */
    HAL_DMA_FLOW_P2P_DSTPERIPH        = 4,    /* Source peripheral to destination peripheral - destination peripheral control */
    HAL_DMA_FLOW_M2P_PERIPH           = 5,    /* Memory to peripheral - peripheral control */
    HAL_DMA_FLOW_P2M_PERIPH           = 6,    /* Peripheral to memory - peripheral control */
    HAL_DMA_FLOW_P2P_SRCPERIPH        = 7,    /* Source peripheral to destination peripheral - source peripheral control */
};

/**
 * DMA Burst size in Source and Destination definitions
 */
enum HAL_DMA_BSIZE_T {
    HAL_DMA_BSIZE_1           = 0,    /* Burst size = 1 */
    HAL_DMA_BSIZE_4           = 1,    /* Burst size = 4 */
    HAL_DMA_BSIZE_8           = 2,    /* Burst size = 8 */
    HAL_DMA_BSIZE_16          = 3,    /* Burst size = 16 */
    HAL_DMA_BSIZE_32          = 4,    /* Burst size = 32 */
    HAL_DMA_BSIZE_64          = 5,    /* Burst size = 64 */
    HAL_DMA_BSIZE_128         = 6,    /* Burst size = 128 */
    HAL_DMA_BSIZE_256         = 7,    /* Burst size = 256 */
};

/**
 * Width in Source transfer width and Destination transfer width definitions
 */
enum HAL_DMA_WDITH_T {
    HAL_DMA_WIDTH_BYTE        = 0,    /* Width = 1 byte */
    HAL_DMA_WIDTH_HALFWORD    = 1,    /* Width = 2 bytes */
    HAL_DMA_WIDTH_WORD        = 2,    /* Width = 4 bytes */
};

enum HAL_DMA_PERIPH_T {
    HAL_GPDMA_SDIO              = 0,
    HAL_GPDMA_SDMMC             = 1,
    HAL_GPDMA_I2C_RX            = 2,
    HAL_GPDMA_I2C_TX            = 3,
    HAL_GPDMA_SPI_RX            = 4,
    HAL_GPDMA_SPI_TX            = 5,
    HAL_GPDMA_SPILCD_RX         = 6,
    HAL_GPDMA_SPILCD_TX         = 7,
    HAL_GPDMA_UART0_RX          = 8,
    HAL_GPDMA_UART0_TX          = 9,
    HAL_GPDMA_UART1_RX          = 10,
    HAL_GPDMA_UART1_TX          = 11,
    HAL_GPDMA_ISPI_TX           = 12,
    HAL_GPDMA_ISPI_RX           = 13,
    HAL_GPDMA_PERIPH_QTY,

    HAL_AUDMA_CODEC_RX          = 0,
    HAL_AUDMA_CODEC_TX          = 1,
    HAL_AUDMA_BTPCM_RX          = 2,
    HAL_AUDMA_BTPCM_TX          = 3,
    HAL_AUDMA_I2S_RX            = 4,
    HAL_AUDMA_I2S_TX            = 5,
    HAL_AUDMA_DPD_RX            = 6,
    HAL_AUDMA_DPD_TX            = 7,
    HAL_AUDMA_SPDIF_RX          = 8,
    HAL_AUDMA_SPDIF_TX          = 9,
    HAL_AUDMA_PERIPH_QTY,
};

struct HAL_DMA_DESC_T;

typedef void (*HAL_DMA_IRQ_HANDLER_T)(uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli);

typedef void (*HAL_DMA_DELAY_FUNC)(uint32_t ms);

/**
 * DMA structure using for DMA configuration
 */
struct HAL_DMA_CH_CFG_T {
    uint8_t ch;                        /* DMA channel number */
    uint8_t try_burst;
    uint16_t src_tsize;                /* Length/Size of transfer */
    enum HAL_DMA_WDITH_T src_width;
    enum HAL_DMA_WDITH_T dst_width;
    enum HAL_DMA_BSIZE_T src_bsize;
    enum HAL_DMA_BSIZE_T dst_bsize;
    enum HAL_DMA_FLOW_CONTROL_T type;    /* Transfer Type */
    enum HAL_DMA_PERIPH_T src_periph;
    enum HAL_DMA_PERIPH_T dst_periph;
    uint32_t src;                    /* Physical Source Address */
    uint32_t dst;                    /* Physical Destination Address */
    HAL_DMA_IRQ_HANDLER_T handler;
};


/**
 * Transfer Descriptor structure typedef
 */
struct HAL_DMA_DESC_T {
    uint32_t src;    /* Source address */
    uint32_t dst;    /* Destination address */
    uint32_t lli;    /* Pointer to next descriptor structure */
    uint32_t ctrl;    /* Control word that has transfer size, type etc. */
};


//=============================================================

void hal_audma_open(void);

void hal_audma_close(void);

bool hal_dma_busy(void);

void hal_audma_sleep(void);

bool hal_gpdma_chan_busy(uint8_t ch);

uint8_t hal_audma_get_chan(enum HAL_DMA_GET_CHAN_T policy);

void hal_audma_free_chan(uint8_t ch);

uint32_t hal_audma_cancel(uint8_t ch);

uint32_t hal_audma_stop(uint8_t ch);

enum HAL_DMA_RET_T hal_audma_start(const struct HAL_DMA_CH_CFG_T *cfg);

enum HAL_DMA_RET_T hal_audma_init_desc(struct HAL_DMA_DESC_T *desc,
                                       const struct HAL_DMA_CH_CFG_T *cfg,
                                       const struct HAL_DMA_DESC_T *next,
                                       int tc_irq);

enum HAL_DMA_RET_T hal_audma_sg_start(const struct HAL_DMA_DESC_T *desc,
                                      const struct HAL_DMA_CH_CFG_T *cfg);

uint32_t hal_audma_get_cur_src_addr(uint8_t ch);

uint32_t hal_audma_get_cur_dst_addr(uint8_t ch);


//=============================================================

void hal_gpdma_open(void);

void hal_gpdma_close(void);

void hal_gpdma_sleep(void);

bool hal_gpdma_chan_busy(uint8_t ch);

uint8_t hal_gpdma_get_chan(enum HAL_DMA_GET_CHAN_T policy);

void hal_gpdma_free_chan(uint8_t ch);

uint32_t hal_gpdma_cancel(uint8_t ch);

uint32_t hal_gpdma_stop(uint8_t ch);

enum HAL_DMA_RET_T hal_gpdma_start(const struct HAL_DMA_CH_CFG_T *cfg);

enum HAL_DMA_RET_T hal_gpdma_init_desc(struct HAL_DMA_DESC_T *desc,
                                       const struct HAL_DMA_CH_CFG_T *cfg,
                                       const struct HAL_DMA_DESC_T *next,
                                       int tc_irq);

enum HAL_DMA_RET_T hal_gpdma_sg_start(const struct HAL_DMA_DESC_T *desc,
                                      const struct HAL_DMA_CH_CFG_T *cfg);

uint32_t hal_gpdma_get_sg_remain_size(uint8_t ch);

void hal_gpdma_irq_run_chan(uint8_t ch);

HAL_DMA_DELAY_FUNC hal_dma_set_delay_func(HAL_DMA_DELAY_FUNC new_func);

#ifdef __cplusplus
}
#endif

#endif
