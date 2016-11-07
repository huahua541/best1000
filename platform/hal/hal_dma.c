#include "reg_dma.h"
#include "hal_dma.h"
#include "hal_trace.h"
#include "cmsis_nvic.h"
#include "hal_timer.h"


enum HAL_DMA_INST_T {
    HAL_DMA_INST_AUDMA,
    HAL_DMA_INST_GPDMA,
    HAL_DMA_INST_QTY
};

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

static struct DMA_T * const dma[HAL_DMA_INST_QTY] = {
    (struct DMA_T *)0x40120000,
    (struct DMA_T *)0x40130000
};

static const IRQn_Type irq_type[HAL_DMA_INST_QTY] = {
    AUDMA_IRQn,
    GPDMA_IRQn
};

static void hal_audma_irq_handler(void);
static void hal_gpdma_irq_handler(void);
static const uint32_t irq_entry[HAL_DMA_INST_QTY] = {
    (uint32_t)hal_audma_irq_handler,
    (uint32_t)hal_gpdma_irq_handler,
};

static const uint32_t audma_fifo_addr[HAL_AUDMA_PERIPH_QTY] = {
    0x4000A1C0, // CODEC RX
    0x4000A1C8, // CODEC TX
    0x4001A1C0, // BTPCM RX
    0x4001A1C8, // BTPCM TX
    0x4001B1C0, // I2S RX
    0x4001B1C8, // I2S TX
    0x40160034, // DPD RX
    0x40170034, // DPD TX
    0x400201C0, // SPDIF RX
    0x400201C8, // SPDIF TX
};

static const uint32_t gpdma_fifo_addr[HAL_GPDMA_PERIPH_QTY] = {
    0x40100200, // SDIO
    0x40110200, // SDMMC
    0x40005010, // I2C RX
    0x40005010, // I2C TX
    0x40006008, // SPI RX
    0x40006008, // SPI TX
    0x40007008, // SPILCD RX
    0x40007008, // SPILCD TX
    0x40008000, // UART0 RX
    0x40008000, // UART0 TX
    0x40009000, // UART1 RX
    0x40009000, // UART1 TX
    0x4001E008, // ISPI TX
    0x4001E008, // ISPI RX
};

static const uint32_t * const fifo[HAL_DMA_INST_QTY] = {
    audma_fifo_addr,
    gpdma_fifo_addr,
};

/* Channel array to monitor free channel */
static bool chan_enabled[HAL_DMA_INST_QTY][DMA_NUMBER_CHANNELS];

static HAL_DMA_IRQ_HANDLER_T handler[HAL_DMA_INST_QTY][DMA_NUMBER_CHANNELS];

static const char * const err_invalid_chan[HAL_DMA_INST_QTY] = {
    "Invalid AUDMA channel: %d",
    "Invalid GPDMA channel: %d",
};

static HAL_DMA_DELAY_FUNC dma_delay = NULL;


/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/
static void hal_dma_delay(uint32_t ms)
{
	if (dma_delay) {
		dma_delay(ms);
	} else {
		hal_sys_timer_delay(MS_TO_TICKS(ms));
	}
}

/* Initialize the DMA */
static void hal_dma_open(enum HAL_DMA_INST_T inst)
{
    uint8_t i;

    /* Reset all channel configuration register */
    for (i = 0; i < DMA_NUMBER_CHANNELS; i++) {
        dma[inst]->CH[i].CONFIG = 0;
    }

    /* Clear all DMA interrupt and error flag */
    dma[inst]->INTTCCLR = ~0;
    dma[inst]->INTERRCLR = ~0;

    /* Reset all channels are free */
    for (i = 0; i < DMA_NUMBER_CHANNELS; i++) {
        chan_enabled[inst][i] = false;
    }

    NVIC_SetVector(irq_type[inst], irq_entry[inst]);
    if (inst == HAL_DMA_INST_AUDMA) {
        NVIC_SetPriority(irq_type[inst], IRQ_PRIORITY_ABOVENORMAL);
    } else {
        NVIC_SetPriority(irq_type[inst], IRQ_PRIORITY_NORMAL);
    }
    NVIC_EnableIRQ(irq_type[inst]);
}

/* Shutdown the DMA */
static void hal_dma_close(enum HAL_DMA_INST_T inst)
{
    NVIC_DisableIRQ(irq_type[inst]);
    dma[inst]->DMACONFIG = 0;
}

/* Put the DMA into sleep mode */
static void hal_dma_sleep(enum HAL_DMA_INST_T inst)
{
    uint8_t ch = 0;

    for (ch = 0; ch < DMA_NUMBER_CHANNELS; ch++) {
        if (dma[inst]->ENBLDCHNS & DMA_STAT_CHAN(ch)) {
            break;
        }
    }

    if (ch == DMA_NUMBER_CHANNELS) {
        dma[inst]->DMACONFIG = 0;
    }
}

static uint32_t hal_dma_cancel(enum HAL_DMA_INST_T inst, uint8_t ch)
{
    uint32_t remains;

    ASSERT(ch < DMA_NUMBER_CHANNELS, err_invalid_chan[inst], ch);

    dma[inst]->CH[ch].CONFIG &= ~DMA_CONFIG_EN;
    dma[inst]->INTTCCLR = DMA_STAT_CHAN(ch);
    dma[inst]->INTERRCLR = DMA_STAT_CHAN(ch);

    remains = GET_BITFIELD(dma[inst]->CH[ch].CONTROL, DMA_CONTROL_TRANSFERSIZE);

    return remains;
}

HAL_DMA_DELAY_FUNC hal_dma_set_delay_func(HAL_DMA_DELAY_FUNC new_func)
{
    HAL_DMA_DELAY_FUNC old_func = dma_delay;
    dma_delay = new_func;
    return old_func;
}

/* Stop a stream DMA transfer */
static uint32_t hal_dma_stop(enum HAL_DMA_INST_T inst, uint8_t ch)
{
	uint8_t retry = 10;
    ASSERT(ch < DMA_NUMBER_CHANNELS, err_invalid_chan[inst], ch);

    dma[inst]->CH[ch].CONFIG |= DMA_CONFIG_HALT;
#if 1	
    while ((dma[inst]->CH[ch].CONFIG & DMA_CONFIG_ACTIVE) && ((--retry)>0))
		hal_dma_delay(1);
#else
	while (dma[inst]->CH[ch].CONFIG & DMA_CONFIG_ACTIVE);
#endif
    return hal_dma_cancel(inst, ch);
}

static bool hal_dma_chan_busy(enum HAL_DMA_INST_T inst, uint8_t ch)
{
    return !!(dma[inst]->ENBLDCHNS & DMA_STAT_CHAN(ch));
}

/* Get a free DMA channel for one DMA connection */
static uint8_t hal_dma_get_chan(enum HAL_DMA_INST_T inst, enum HAL_DMA_GET_CHAN_T policy)
{
    uint8_t i, ch;
    uint8_t got = HAL_DMA_CHAN_NONE;
    uint32_t lock;

    ASSERT(policy == HAL_DMA_HIGH_PRIO || policy == HAL_DMA_LOW_PRIO || policy == HAL_DMA_LOW_PRIO_ONLY,
        "Invalid DMA policy: %d", policy);

    lock = int_lock();
    for (i = 0; i < DMA_NUMBER_CHANNELS; i++) {
        if (policy == HAL_DMA_HIGH_PRIO) {
            ch = i;
        } else if (policy == HAL_DMA_LOW_PRIO) {
            ch = DMA_NUMBER_CHANNELS - 1 - i;
        } else {
            ch = DMA_NUMBER_CHANNELS - 1 - i;
            if (ch < 6) {
                break;
            }
        }

        if (!hal_dma_chan_busy(inst, ch) && !chan_enabled[inst][ch]) {
            chan_enabled[inst][ch] = true;
            got = ch;
            break;
        }
    }
    int_unlock(lock);

    return got;
}

static void hal_dma_free_chan(enum HAL_DMA_INST_T inst, uint8_t ch)
{
    uint32_t lock;

    hal_dma_cancel(inst, ch);

    lock = int_lock();
    chan_enabled[inst][ch] = false;
    for (ch = 0; ch < DMA_NUMBER_CHANNELS; ch++) {
        if (chan_enabled[inst][ch]) {
            break;
        }
    }
    if (ch == DMA_NUMBER_CHANNELS) {
        dma[inst]->DMACONFIG = 0;
    }
    int_unlock(lock);
}

static enum HAL_DMA_RET_T hal_dma_init_control(enum HAL_DMA_INST_T inst,
                                               uint32_t *ctrl,
                                               const struct HAL_DMA_CH_CFG_T *cfg,
                                               int tc_irq)
{
    uint32_t addr_inc;

    if (cfg->src_tsize > MAX_TRANSFERSIZE) {
        return HAL_DMA_ERR;
    }

    switch (cfg->type) {
        case HAL_DMA_FLOW_M2M_DMA:
            addr_inc = DMA_CONTROL_SI | DMA_CONTROL_DI;
            break;
        case HAL_DMA_FLOW_M2P_DMA:
        case HAL_DMA_FLOW_M2P_PERIPH:
            addr_inc = DMA_CONTROL_SI;
            break;
        case HAL_DMA_FLOW_P2M_DMA:
        case HAL_DMA_FLOW_P2M_PERIPH:
            addr_inc = DMA_CONTROL_DI;
            break;
        case HAL_DMA_FLOW_P2P_DMA:
        case HAL_DMA_FLOW_P2P_DSTPERIPH:
        case HAL_DMA_FLOW_P2P_SRCPERIPH:
            addr_inc = 0;
        default:
            return HAL_DMA_ERR;
    }

    *ctrl = DMA_CONTROL_TRANSFERSIZE(cfg->src_tsize) |
            DMA_CONTROL_SBSIZE(cfg->src_bsize) |
            DMA_CONTROL_DBSIZE(cfg->dst_bsize) |
            DMA_CONTROL_SWIDTH(cfg->src_width) |
            DMA_CONTROL_DWIDTH(cfg->dst_width) |
            (tc_irq ? DMA_CONTROL_TC_IRQ : 0) |
            addr_inc;

    return HAL_DMA_OK;
}

static enum HAL_DMA_RET_T hal_dma_init_desc(enum HAL_DMA_INST_T inst,
                                            struct HAL_DMA_DESC_T *desc,
                                            const struct HAL_DMA_CH_CFG_T *cfg,
                                            const struct HAL_DMA_DESC_T *next,
                                            int tc_irq)
{
    uint32_t ctrl;
    enum HAL_DMA_RET_T ret;

    ret = hal_dma_init_control(inst, &ctrl, cfg, tc_irq);
    if (ret != HAL_DMA_OK) {
        return ret;
    }

    if (cfg->type == HAL_DMA_FLOW_M2M_DMA || cfg->type == HAL_DMA_FLOW_M2P_DMA ||
            cfg->type == HAL_DMA_FLOW_M2P_PERIPH) {
        desc->src = cfg->src;
    } else {
        desc->src = fifo[inst][cfg->src_periph];
    }
    if (cfg->type == HAL_DMA_FLOW_M2M_DMA || cfg->type == HAL_DMA_FLOW_P2M_DMA ||
            cfg->type == HAL_DMA_FLOW_P2M_PERIPH) {
        desc->dst = cfg->dst;
    } else {
        desc->dst = fifo[inst][cfg->dst_periph];
    }
    desc->lli = (uint32_t)next;
    desc->ctrl = ctrl;

    return HAL_DMA_OK;
}

/* Do a DMA scatter-gather transfer M2M, M2P,P2M or P2P using DMA descriptors */
static enum HAL_DMA_RET_T hal_dma_sg_start(enum HAL_DMA_INST_T inst,
                                           const struct HAL_DMA_DESC_T *desc,
                                           const struct HAL_DMA_CH_CFG_T *cfg)
{
    uint32_t irq_mask, try_burst;

    ASSERT(cfg->ch < DMA_NUMBER_CHANNELS, err_invalid_chan[inst], cfg->ch);

    if (!chan_enabled[inst][cfg->ch]) {
        // Not acquired
        return HAL_DMA_ERR;
    }
    if (dma[inst]->ENBLDCHNS & DMA_STAT_CHAN(cfg->ch)) {
        // Busy
        return HAL_DMA_ERR;
    }

    if (cfg->handler == NULL) {
        irq_mask = 0;
    } else {
        irq_mask = DMA_CONFIG_ERR_IRQMASK | DMA_CONFIG_TC_IRQMASK;
        handler[inst][cfg->ch] = cfg->handler;
    }

    try_burst = cfg->try_burst ? DMA_CONFIG_TRY_BURST : 0;

    /* Reset the Interrupt status */
    dma[inst]->INTTCCLR = DMA_STAT_CHAN(cfg->ch);
    dma[inst]->INTERRCLR = DMA_STAT_CHAN(cfg->ch);

    dma[inst]->CH[cfg->ch].SRCADDR = desc->src;
    dma[inst]->CH[cfg->ch].DSTADDR = desc->dst;
    dma[inst]->CH[cfg->ch].LLI = desc->lli;
    dma[inst]->CH[cfg->ch].CONTROL = desc->ctrl;
    dma[inst]->CH[cfg->ch].CONFIG = DMA_CONFIG_EN |
                                    DMA_CONFIG_SRCPERIPH(cfg->src_periph) |
                                    DMA_CONFIG_DSTPERIPH(cfg->dst_periph) |
                                    DMA_CONFIG_TRANSFERTYPE(cfg->type) |
                                    irq_mask |
                                    try_burst;
    dma[inst]->DMACONFIG = DMA_DMACONFIG_EN;

    return HAL_DMA_OK;
}

/* Do a DMA transfer M2M, M2P,P2M or P2P */
static enum HAL_DMA_RET_T hal_dma_start(enum HAL_DMA_INST_T inst, const struct HAL_DMA_CH_CFG_T *cfg)
{
    struct HAL_DMA_DESC_T desc;
    enum HAL_DMA_RET_T ret;

    ASSERT(cfg->ch < DMA_NUMBER_CHANNELS, err_invalid_chan[inst], cfg->ch);

    ret = hal_dma_init_desc(inst, &desc, cfg, NULL, 1);
    if (ret != HAL_DMA_OK) {
        return ret;
    }

    ret = hal_dma_sg_start(inst, &desc, cfg);
    if (ret != HAL_DMA_OK) {
        return ret;
    }

    return HAL_DMA_OK;
}

static uint32_t hal_dma_get_cur_src_addr(enum HAL_DMA_INST_T inst, uint8_t ch)
{
    return dma[inst]->CH[ch].SRCADDR;
}

static uint32_t hal_dma_get_cur_dst_addr(enum HAL_DMA_INST_T inst, uint8_t ch)
{
    return dma[inst]->CH[ch].DSTADDR;
}

static void hal_dma_irq_run_chan(enum HAL_DMA_INST_T inst, uint8_t ch)
{
    uint32_t remains;
    struct HAL_DMA_DESC_T *lli;
    bool tcint, errint;

    if ((dma[inst]->INTSTAT & DMA_STAT_CHAN(ch)) == 0) {
        return;
    }

    /* Check counter terminal status */
    tcint = !!(dma[inst]->INTTCSTAT & DMA_STAT_CHAN(ch));
    /* Check error terminal status */
    errint = !!(dma[inst]->INTERRSTAT & DMA_STAT_CHAN(ch));

    if (tcint || errint) {
        if (tcint) {
            /* Clear terminate counter Interrupt pending */
            dma[inst]->INTTCCLR = DMA_STAT_CHAN(ch);
        }
        if (errint) {
            /* Clear error counter Interrupt pending */
            dma[inst]->INTERRCLR = DMA_STAT_CHAN(ch);
        }

        if (handler[inst][ch]) {
            remains = GET_BITFIELD(dma[inst]->CH[ch].CONTROL, DMA_CONTROL_TRANSFERSIZE);
            lli = (struct HAL_DMA_DESC_T *)dma[inst]->CH[ch].LLI;
            handler[inst][ch](ch, remains, errint, lli);
        }
    }
}

static void hal_dma_irq_handler(enum HAL_DMA_INST_T inst)
{
    int ch;
    uint32_t remains;
    struct HAL_DMA_DESC_T *lli;
    bool tcint, errint;

    for (ch = 0; ch < DMA_NUMBER_CHANNELS; ch++) {
        if ((dma[inst]->INTSTAT & DMA_STAT_CHAN(ch)) == 0) {
            continue;
        }

        /* Check counter terminal status */
        tcint = !!(dma[inst]->INTTCSTAT & DMA_STAT_CHAN(ch));
        /* Check error terminal status */
        errint = !!(dma[inst]->INTERRSTAT & DMA_STAT_CHAN(ch));

        if (tcint || errint) {
            if (tcint) {
                /* Clear terminate counter Interrupt pending */
                dma[inst]->INTTCCLR = DMA_STAT_CHAN(ch);
            }
            if (errint) {
                /* Clear error counter Interrupt pending */
                dma[inst]->INTERRCLR = DMA_STAT_CHAN(ch);
            }

            if (handler[inst][ch]) {
                remains = GET_BITFIELD(dma[inst]->CH[ch].CONTROL, DMA_CONTROL_TRANSFERSIZE);
                lli = (struct HAL_DMA_DESC_T *)dma[inst]->CH[ch].LLI;
                handler[inst][ch](ch, remains, errint, lli);
            }
        }
    }
}

/*****************************************************************************
 * Audio DMA Public functions
 ****************************************************************************/

void hal_audma_open(void)
{
    hal_dma_open(HAL_DMA_INST_AUDMA);
}

void hal_audma_close(void)
{
    hal_dma_close(HAL_DMA_INST_AUDMA);
}

bool hal_dma_busy(void)
{
	uint8_t inst,i;
	bool dma_busy = false;
	
	for (inst = 0; inst < HAL_DMA_INST_QTY-1; inst++){
		for (i = 0; i < DMA_NUMBER_CHANNELS; i++) {
			if (chan_enabled[inst][i]){
				dma_busy= true;
			}
		}
	}
	return dma_busy;
}

void hal_audma_sleep(void)
{
    hal_dma_sleep(HAL_DMA_INST_AUDMA);
}

bool hal_audma_chan_busy(uint8_t ch)
{
    return hal_dma_chan_busy(HAL_DMA_INST_AUDMA, ch);
}

uint8_t hal_audma_get_chan(enum HAL_DMA_GET_CHAN_T policy)
{
    return hal_dma_get_chan(HAL_DMA_INST_AUDMA, policy);
}

void hal_audma_free_chan(uint8_t ch)
{
    hal_dma_free_chan(HAL_DMA_INST_AUDMA, ch);
}

uint32_t hal_audma_cancel(uint8_t ch)
{
    return hal_dma_cancel(HAL_DMA_INST_AUDMA, ch);
}

uint32_t hal_audma_stop(uint8_t ch)
{
    return hal_dma_stop(HAL_DMA_INST_AUDMA, ch);
}

enum HAL_DMA_RET_T hal_audma_start(const struct HAL_DMA_CH_CFG_T *cfg)
{
    return hal_dma_start(HAL_DMA_INST_AUDMA, cfg);
}

enum HAL_DMA_RET_T hal_audma_init_desc(struct HAL_DMA_DESC_T *desc,
                                       const struct HAL_DMA_CH_CFG_T *cfg,
                                       const struct HAL_DMA_DESC_T *next,
                                       int tc_irq)
{
    return hal_dma_init_desc(HAL_DMA_INST_AUDMA, desc, cfg, next, tc_irq);
}

enum HAL_DMA_RET_T hal_audma_sg_start(const struct HAL_DMA_DESC_T *desc,
                                      const struct HAL_DMA_CH_CFG_T *cfg)
{
    return hal_dma_sg_start(HAL_DMA_INST_AUDMA, desc, cfg);
}

uint32_t hal_audma_get_cur_src_addr(uint8_t ch)
{
    return hal_dma_get_cur_src_addr(HAL_DMA_INST_AUDMA, ch);
}

uint32_t hal_audma_get_cur_dst_addr(uint8_t ch)
{
    return hal_dma_get_cur_dst_addr(HAL_DMA_INST_AUDMA, ch);
}

static void hal_audma_irq_handler(void)
{
    hal_dma_irq_handler(HAL_DMA_INST_AUDMA);
}

/*****************************************************************************
 * General Purpose DMA Public functions
 ****************************************************************************/

void hal_gpdma_open(void)
{
    hal_dma_open(HAL_DMA_INST_GPDMA);
}

void hal_gpdma_close(void)
{
    hal_dma_close(HAL_DMA_INST_GPDMA);
}

void hal_gpdma_sleep(void)
{
    hal_dma_sleep(HAL_DMA_INST_GPDMA);
}

bool hal_gpdma_chan_busy(uint8_t ch)
{
    return hal_dma_chan_busy(HAL_DMA_INST_GPDMA, ch);
}

uint8_t hal_gpdma_get_chan(enum HAL_DMA_GET_CHAN_T policy)
{
    return hal_dma_get_chan(HAL_DMA_INST_GPDMA, policy);
}

void hal_gpdma_free_chan(uint8_t ch)
{
    hal_dma_free_chan(HAL_DMA_INST_GPDMA, ch);
}

uint32_t hal_gpdma_cancel(uint8_t ch)
{
    return hal_dma_cancel(HAL_DMA_INST_GPDMA, ch);
}

uint32_t hal_gpdma_stop(uint8_t ch)
{
    return hal_dma_stop(HAL_DMA_INST_GPDMA, ch);
}

enum HAL_DMA_RET_T hal_gpdma_start(const struct HAL_DMA_CH_CFG_T *cfg)
{
    return hal_dma_start(HAL_DMA_INST_GPDMA, cfg);
}

enum HAL_DMA_RET_T hal_gpdma_init_desc(struct HAL_DMA_DESC_T *desc,
                                       const struct HAL_DMA_CH_CFG_T *cfg,
                                       const struct HAL_DMA_DESC_T *next,
                                       int tc_irq)
{
    return hal_dma_init_desc(HAL_DMA_INST_GPDMA, desc, cfg, next, tc_irq);
}

enum HAL_DMA_RET_T hal_gpdma_sg_start(const struct HAL_DMA_DESC_T *desc,
                                      const struct HAL_DMA_CH_CFG_T *cfg)
{
    return hal_dma_sg_start(HAL_DMA_INST_GPDMA, desc, cfg);
}

uint32_t hal_gpdma_get_sg_remain_size(uint8_t ch)
{
    uint32_t remains;
    const struct HAL_DMA_DESC_T *desc;

    remains = GET_BITFIELD(dma[HAL_DMA_INST_GPDMA]->CH[ch].CONTROL, DMA_CONTROL_TRANSFERSIZE);
    desc = (const struct HAL_DMA_DESC_T *)dma[HAL_DMA_INST_GPDMA]->CH[ch].LLI;
    while (desc) {
        remains += GET_BITFIELD(desc->ctrl, DMA_CONTROL_TRANSFERSIZE);
        desc = (const struct HAL_DMA_DESC_T *)desc->lli;
    }

    return remains;
}

void hal_gpdma_irq_run_chan(uint8_t ch)
{
    hal_dma_irq_run_chan(HAL_DMA_INST_GPDMA, ch);
}

static void hal_gpdma_irq_handler(void)
{
    hal_dma_irq_handler(HAL_DMA_INST_GPDMA);
}

