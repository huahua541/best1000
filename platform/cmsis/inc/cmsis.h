/* mbed Microcontroller Library - CMSIS
 * Copyright (C) 2009-2011 ARM Limited. All rights reserved.
 *
 * A generic CMSIS include header, pulling in LPC11U24 specifics
 */

#ifndef MBED_CMSIS_H
#define MBED_CMSIS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "best1000.h"

#define IRQ_PRIORITY_HIGH                   3
#define IRQ_PRIORITY_ABOVENORMAL            4
#define IRQ_PRIORITY_NORMAL                 5

static inline uint32_t int_lock(void)
{
	uint32_t pri = __get_PRIMASK();
	if ((pri & 0x1) == 0) {
		__disable_irq();
	}
	return pri;
}

static inline void int_unlock(uint32_t pri)
{
	if ((pri & 0x1) == 0) {
		__enable_irq();
	}
}

#ifdef __cplusplus
}
#endif

#endif
