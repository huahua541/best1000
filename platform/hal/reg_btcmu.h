#ifndef __REG_BTCMU_H__
#define __REG_BTCMU_H__

#include "plat_types.h"

struct BTCMU_T {
         uint32_t RESERVED[0x50 / 4];
    __IO uint32_t ISIRQ_SET;        // 0x50
    __IO uint32_t ISIRQ_CLR;        // 0x54
};

// ISIRQ_SET
#define BTCMU_MCU2BT_DATA_DONE_SET      (1 << 0)
#define BTCMU_MCU2BT_DATA1_DONE_SET     (1 << 1)
#define BTCMU_BT2MCU_DATA_IND_SET       (1 << 2)
#define BTCMU_BT2MCU_DATA1_IND_SET      (1 << 3)
#define BTCMU_MCU_ALLIRQ_MASK_SET       (1 << 4)

// ISIRQ_CLR
#define BTCMU_MCU2BT_DATA_DONE_CLR      (1 << 0)
#define BTCMU_MCU2BT_DATA1_DONE_CLR     (1 << 1)
#define BTCMU_BT2MCU_DATA_IND_CLR       (1 << 2)
#define BTCMU_BT2MCU_DATA1_IND_CLR      (1 << 3)
#define BTCMU_MCU_ALLIRQ_MASK_CLR       (1 << 4)

#endif

