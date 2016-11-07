#ifndef NORFLASHIP_HAL_H
#define NORFLASHIP_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "reg_norflaship.h"
#include "task_schedule.h"

/* register memory address */
#define NORFLASHIP_BASEADDR 0x40140000

/* control clk size in bytes each time to read data , it is max value of BLKSIZE */
/* see TX_BLKSIZE_MASK register bits width */
#define NORFLASHIP_ONCE_READ_SIZE (256)

/* read(rx) fifo size in bytes */
#define NORFLASHIP_RXFIFO_SIZE_MAX (8)

#define norflaship_readb(a) \
     (*(volatile unsigned char*)(NORFLASHIP_BASEADDR+a))

#define norflaship_read32(a) \
     (*(volatile unsigned int *)(NORFLASHIP_BASEADDR+a))

#define norflaship_write32(v,a) \
     ((*(volatile unsigned int *)(NORFLASHIP_BASEADDR+a)) = v)


/* ip ops */
static inline uint8_t norflaship_continuous_read_mode_bit(uint8_t mode_bit)
{
    uint32_t val = 0;
    val = norflaship_read32(TX_CONFIG2_BASE);
    val &= ~(TX_MODBIT_MASK);
    val |= (mode_bit<<TX_MODBIT_SHIFT)&TX_MODBIT_MASK;
    norflaship_write32(val, TX_CONFIG2_BASE);
    return 0;
}
static inline uint8_t norflaship_continuous_read_off(void)
{
    uint32_t val = 0;
    val = norflaship_read32(TX_CONFIG2_BASE);
    val &= ~(TX_CONMOD_MASK);
    norflaship_write32(val, TX_CONFIG2_BASE);
    return 0;
}
static inline uint8_t norflaship_continuous_read_on(void)
{
    uint32_t val = 0;
    val = norflaship_read32(TX_CONFIG2_BASE);
    val |= (TX_CONMOD_MASK);
    norflaship_write32(val, TX_CONFIG2_BASE);
    return 0;
}
static inline uint8_t norflaship_write_txfifo(const uint8_t *val, uint32_t len)
{
    uint32_t i = 0;
    for (i = 0; i < len; ++i) {
        norflaship_write32(val[i], TXDATA_BASE);
    }
    return 0;
}
static inline uint8_t norflaship_read_rxfifo_count(void)
{
    uint32_t val = 0;
    val = norflaship_readb(INT_STATUS_BASE);
    return ((val&RXFIFOCOUNT_MASK)>>RXFIFOCOUNT_SHIFT);
}
static inline uint8_t norflaship_read_rxfifo(void)
{
    uint32_t val = 0;
    val = norflaship_readb(RXDATA_BASE);
    return val&0xff;
}
static inline void norflaship_blksize(uint32_t blksize)
{
    uint32_t val = 0;
    val = norflaship_read32(TX_CONFIG2_BASE);
    val = ((~(TX_BLKSIZE_MASK))&val) | (blksize<<TX_BLKSIZE_SHIFT);
    norflaship_write32(val, TX_CONFIG2_BASE);
}
static inline void norflaship_cmd_addr(uint32_t cmd, uint32_t address)
{
    norflaship_write32(address<<TX_ADDR_SHIFT|(cmd<<TX_CMD_SHIFT), TX_CONFIG1_BASE);
}
static inline void norflaship_rxfifo_count_wait(uint8_t cnt)
{
    uint32_t st = 0;
    do {
        st = norflaship_read32(INT_STATUS_BASE);
    } while ((((st&RXFIFOCOUNT_MASK)>>RXFIFOCOUNT_SHIFT) != cnt) && TASK_SCHEDULE);
}
static inline void norflaship_rxfifo_empty_wait(void)
{
    uint32_t st = 0;
    do {
        st = norflaship_read32(INT_STATUS_BASE);
    } while ((st&RXFIFOEMPTY_MASK) && TASK_SCHEDULE);
}
static inline void norflaship_busy_wait(void)
{
    uint32_t st = 0;
    do {
        st = norflaship_read32(INT_STATUS_BASE);
    } while ((st&BUSY_MASK) && TASK_SCHEDULE);
}
static inline void norflaship_clear_fifos(void)
{
    norflaship_write32(RXFIFOCLR_MASK|TXFIFOCLR_MASK, FIFO_CONFIG_BASE);
}
static inline void norflaship_clear_rxfifo(void)
{
    norflaship_write32(RXFIFOCLR_MASK, FIFO_CONFIG_BASE);
}
static inline void norflaship_clear_txfifo(void)
{
    norflaship_write32(TXFIFOCLR_MASK, FIFO_CONFIG_BASE);
}
static inline void norflaship_div(uint32_t div)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE1_CONFIG_BASE);
    val = (~(CLKDIV_MASK) & val) | (div<<CLKDIV_SHIFT);
    norflaship_write32(val, MODE1_CONFIG_BASE);
}
static inline void norflaship_cmdquad(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE1_CONFIG_BASE);
    if (v)
        val |= CMDQUAD_MASK;
    else
        val &= ~CMDQUAD_MASK;
    norflaship_write32(val, MODE1_CONFIG_BASE);
}
static inline void norflaship_pos_neg(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE1_CONFIG_BASE);
    if (v)
        val |= POS_NEG_MASK;
    else
        val &= ~POS_NEG_MASK;
    norflaship_write32(val, MODE1_CONFIG_BASE);
}
static inline void norflaship_neg_phase(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE1_CONFIG_BASE);
    if (v)
        val |= NEG_PHASE_MASK;
    else
        val &= ~NEG_PHASE_MASK;
    norflaship_write32(val, MODE1_CONFIG_BASE);
}
static inline void norflaship_samdly(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE1_CONFIG_BASE);
    val = (~(SAMDLY_MASK) & val) | (v<<SAMDLY_SHIFT);
    norflaship_write32(val, MODE1_CONFIG_BASE);
}
static inline void norflaship_dual_mode(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE1_CONFIG_BASE);
    if (v)
        val |= DUALMODE_MASK;
    else
        val &= ~DUALMODE_MASK;
    norflaship_write32(val, MODE1_CONFIG_BASE);
}
static inline void norflaship_hold_pin(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE1_CONFIG_BASE);
    if (v)
        val |= HOLDPIN_MASK;
    else
        val &= ~HOLDPIN_MASK;
    norflaship_write32(val, MODE1_CONFIG_BASE);
}
static inline void norflaship_wpr_pin(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE1_CONFIG_BASE);
    if (v)
        val |= WPRPIN_MASK;
    else
        val &= ~WPRPIN_MASK;
    norflaship_write32(val, MODE1_CONFIG_BASE);
}
static inline void norflaship_quad_mode(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE1_CONFIG_BASE);
    if (v)
        val |= QUADMODE_MASK;
    else
        val &= ~QUADMODE_MASK;
    norflaship_write32(val, MODE1_CONFIG_BASE);
}
static inline void norflaship_dummyclc(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MISC_CONFIG_BASE);
    val = (~(DUMMYCLC_MASK) & val) | (v<<DUMMYCLC_SHIFT);
    norflaship_write32(val, MISC_CONFIG_BASE);
}
static inline void norflaship_dummyclcen(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MISC_CONFIG_BASE);
    if (v)
        val |= DUMMYCLCEN_MASK;
    else
        val &= ~DUMMYCLCEN_MASK;
    norflaship_write32(val, MISC_CONFIG_BASE);
}
static inline void norflaship_4byteaddr(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MISC_CONFIG_BASE);
    if (v)
        val |= _4BYTEADDR_MASK;
    else
        val &= ~_4BYTEADDR_MASK;
    norflaship_write32(val, MISC_CONFIG_BASE);
}
static inline void norflaship_ruen(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(PULL_UP_DOWN_CONFIG_BASE);
    val = (~(SPIRUEN_MASK) & val) | (v<<SPIRUEN_SHIFT);
    norflaship_write32(val, PULL_UP_DOWN_CONFIG_BASE);
}
static inline void norflaship_rden(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(PULL_UP_DOWN_CONFIG_BASE);
    val = (~(SPIRDEN_MASK) & val) | (v<<SPIRDEN_SHIFT);
    norflaship_write32(val, PULL_UP_DOWN_CONFIG_BASE);
}
static inline void norflaship_dualiocmd(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE2_CONFIG_BASE);
    val = (~(DUALIOCMD_MASK) & val) | (v<<DUALIOCMD_SHIFT);
    norflaship_write32(val, MODE2_CONFIG_BASE);
}
static inline void norflaship_rdcmd(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE2_CONFIG_BASE);
    val = (~(RDCMD_MASK) & val) | (v<<RDCMD_SHIFT);
    norflaship_write32(val, MODE2_CONFIG_BASE);
}
static inline void norflaship_frdcmd(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE2_CONFIG_BASE);
    val = (~(FRDCMD_MASK) & val) | (v<<FRDCMD_SHIFT);
    norflaship_write32(val, MODE2_CONFIG_BASE);
}
static inline void norflaship_qrdcmd(uint32_t v)
{
    uint32_t val = 0;
    val = norflaship_read32(MODE2_CONFIG_BASE);
    val = (~(QRDCMDBIT_MASK) & val) | (v<<QRDCMDBIT_SHIFT);
    norflaship_write32(val, MODE2_CONFIG_BASE);
}

#ifdef __cplusplus
}
#endif

#endif /* NORFLASHIP_HAL_H */
