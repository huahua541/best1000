#ifndef NORFLASH_REG_H
#define NORFLASH_REG_H

#include "plat_types.h"

/* ip register */
/* 0x0 */
#define TX_CONFIG1_BASE 0x0
#define TX_ADDR_SHIFT (8)
#define TX_ADDR_MASK ((0xfffffff)<TX_ADDR_SHIFT)
#define TX_CMD_SHIFT (0)
#define TX_CMD_MASK ((0xf)<TX_CMD_SHIFT)
#define TX_ADDR_RW_SHIFT (16)
#define TX_ADDR_RW_MASK ((0x1)<<TX_ADDR_RW_SHIFT)

/* 0x4 */
#define TX_CONFIG2_BASE 0x4
#define TX_CONMOD_SHIFT (24)
#define TX_CONMOD_MASK ((0x1)<<TX_CONMOD_SHIFT)
#define TX_BLKSIZE_SHIFT (8)
#define TX_BLKSIZE_MASK ((0x1ff)<<TX_BLKSIZE_SHIFT)
#define TX_MODBIT_SHIFT (0)
#define TX_MODBIT_MASK ((0xff)<<TX_MODBIT_SHIFT)

/* 0x8 */
#define TXDATA_BASE 0x8
#define TXDATA_SHIFT (0)
#define TXDATA_MASK ((0xff)<<TXDATA_SHIFT)

/* 0xc */
#define INT_STATUS_BASE 0xc
#define RXFIFOCOUNT_SHIFT (4)
#define RXFIFOCOUNT_MASK ((0x1f)<<RXFIFOCOUNT_SHIFT)
#define RXFIFOEMPTY_SHIFT (3)
#define RXFIFOEMPTY_MASK ((0x1)<<RXFIFOEMPTY_SHIFT)
#define TXFIFOFULL_SHIFT (2)
#define TXFIFOFULL_MASK ((0x1)<<RXFIFOFULL_SHIFT)
#define TXFIFOEMPTY_SHIFT (1)
#define TXFIFOEMPTY_MASK ((0x1)<<TXFIFOEMPTY_SHIFT)
#define BUSY_SHIFT (0)
#define BUSY_MASK ((0x1)<<BUSY_SHIFT)

/* 0x10 */
#define RXDATA_BASE 0x10
#define RXDATA_SHIFT (0)
#define RXDATA_MASK ((0xffffffff)<<RXDATA_SHIFT)

/* 0x14 */
#define MODE1_CONFIG_BASE 0x14
#define NEG_PHASE_SHIFT (18)
#define NEG_PHASE_MASK ((0x1)<<NEG_PHASE_SHIFT)
#define POS_NEG_SHIFT (17)
#define POS_NEG_MASK ((0x1)<<POS_NEG_SHIFT)
#define CMDQUAD_SHIFT (16)
#define CMDQUAD_MASK ((0x1)<<CMDQUAD_SHIFT)
#define CLKDIV_SHIFT (8)
#define CLKDIV_MASK ((0xff)<<CLKDIV_SHIFT)
#define SAMDLY_SHIFT (4)
#define SAMDLY_MASK ((0x7)<<SAMDLY_SHIFT)
#define DUALMODE_SHIFT (3)
#define DUALMODE_MASK ((0x1)<<DUALMODE_SHIFT)
#define HOLDPIN_SHIFT (2)
#define HOLDPIN_MASK ((0x1)<<HOLDPIN_SHIFT)
#define WPRPIN_SHIFT (1)
#define WPRPIN_MASK ((0x1)<<WPRPIN_SHIFT)
#define QUADMODE_SHIFT (0)
#define QUADMODE_MASK ((0x1)<<QUADMODE_SHIFT)

/* 0x18 */
#define FIFO_CONFIG_BASE 0x18
#define TXFIFOCLR_SHIFT (1)
#define TXFIFOCLR_MASK ((0x1)<<TXFIFOCLR_SHIFT)
#define RXFIFOCLR_SHIFT (0)
#define RXFIFOCLR_MASK ((0x1)<<RXFIFOCLR_SHIFT)

/* 0x20 */
#define MODE2_CONFIG_BASE 0x20
#define DUALIOCMD_SHIFT (24)
#define DUALIOCMD_MASK ((0xff)<<DUALIOCMD_SHIFT)
#define RDCMD_SHIFT (16)
#define RDCMD_MASK ((0xff)<<RDCMD_SHIFT)
#define FRDCMD_SHIFT (8)
#define FRDCMD_MASK ((0xff)<<FRDCMD_SHIFT)
#define QRDCMDBIT_SHIFT (0)
#define QRDCMDBIT_MASK ((0xff)<<QRDCMDBIT_SHIFT)

/* 0x2c */
#define MISC_CONFIG_BASE 0x2c
#define DUMMYCLC_SHIFT (8)
#define DUMMYCLC_MASK ((0xf)<<DUMMYCLC_SHIFT)
#define DUMMYCLCEN_SHIFT (1)
#define DUMMYCLCEN_MASK ((0x1)<<DUMMYCLCEN_SHIFT)
#define _4BYTEADDR_SHIFT (0)
#define _4BYTEADDR_MASK ((0x1)<<_4BYTEADDR_SHIFT)

/* 0x34 */
#define PULL_UP_DOWN_CONFIG_BASE 0x34
#define SPIRUEN_SHIFT (4)
#define SPIRUEN_MASK ((0xf)<<SPIRUEN_SHIFT)
#define SPIRDEN_SHIFT (0)
#define SPIRDEN_MASK ((0xf)<<SPIRDEN_SHIFT)

#endif /* NORFLASH_REG_H */

