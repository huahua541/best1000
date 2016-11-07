#ifndef __REG_BTCMU_BEST1000_H__
#define __REG_BTCMU_BEST1000_H__

#include "plat_types.h"

struct BTCMU_T {
         uint32_t RESERVED[0x50 / 4];
    __IO uint32_t ISIRQ_SET;        // 0x50
    __IO uint32_t ISIRQ_CLR;        // 0x54
};

#endif

