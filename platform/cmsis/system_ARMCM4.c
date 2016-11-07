/**************************************************************************//**
 * @file     system_ARMCM4.c
 * @brief    CMSIS Device System Source File for
 *           ARMCM4 Device Series
 * @version  V1.09
 * @date     27. August 2014
 *
 * @note
 *
 ******************************************************************************/
/* Copyright (c) 2011 - 2014 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/

#include "cmsis.h"
#include "hal_cache.h"
#include "hal_cmu.h"
#include "hal_iomuxip.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_analogif.h"
#include "hal_psram.h"
#include "pmu.h"
#include "analog.h"

#define REG(a) *(volatile uint32_t *)(a)

#define REG_SET_FIELD(reg, mask, shift, v)\
                        do{ \
                            *(volatile unsigned int *)(reg) &= ~(mask<<shift); \
                            *(volatile unsigned int *)(reg) |= (v<<shift); \
                        }while(0)

/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System.
 */
void SystemInit (void)
{
#if (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2) |                 /* set CP10 Full Access */
                   (3UL << 11*2)  );               /* set CP11 Full Access */
#endif

#ifdef UNALIGNED_SUPPORT_DISABLE
    SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
#endif
}

// -----------------------------------------------------------
// Boot initialization
// CAUTION: This function and all the functions it called must
//          NOT access global data/bss, for the global data/bss
//          is not available at that time.
// -----------------------------------------------------------
extern uint32_t __boot_sram_start_flash__[];
extern uint32_t __boot_sram_end_flash__[];
extern uint32_t __boot_sram_start__[];
extern uint32_t __boot_bss_sram_start__[];
extern uint32_t __boot_bss_sram_end__[];

extern uint32_t __sram_text_data_start_flash__[];
extern uint32_t __sram_text_data_end_flash__[];
extern uint32_t __sram_text_data_start__[];
extern uint32_t __sram_bss_start__[];
extern uint32_t __sram_bss_end__[];
extern uint32_t __fast_sram_text_data_start__[];
extern uint32_t __fast_sram_text_data_end__[];
extern uint32_t __fast_sram_text_data_start_flash__[];
extern uint32_t __fast_sram_text_data_end_flash__[];

extern uint32_t __psram_text_data_start_flash__[];
extern uint32_t __psram_text_data_end_flash__[];
extern uint32_t __psram_text_data_start__[];
extern uint32_t __psram_bss_start__[];
extern uint32_t __psram_bss_end__[];

void system_psram_init(void)
{
	int ret = 0;
	uint32_t v = 0;
	uint32_t *dst, *src;

	hal_sys_timer_open();

    v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX44_REG_OFFSET);
    v &= ~(1<<2); 
    iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX44_REG_OFFSET);

    ret = hal_analogif_open();

    pmu_open();

    //fix me set digital gain
    REG_SET_FIELD(0x4000a050, 0x07, 24, 4); // [26:24] ch0 adc hbf3_sel 0 for *8 1 for *10 2 for *11 3 for *12 4 for *13 5 for *16
    REG_SET_FIELD(0x4000a050, 0x07, 27, 4); // [29:27] ch1 adc hbf3_sel
    REG_SET_FIELD(0x4000a050, 0x0f,  7, 6); // [10:7]  ch1 adc volume mic gain -12db(0001)~16db(1111) mute(0000)
    REG_SET_FIELD(0x4000a048, 0x0f, 25, 0x06); // // [28:25]  ch0 adc volume mic gain -12db(0001)~16db(1111) mute(0000)
    
    anlog_open();

	hal_psram_init(0);

  	for (dst = __psram_text_data_start__, src = __psram_text_data_start_flash__;
            src < __psram_text_data_end_flash__;
            dst++, src++) {
        *dst = *src;
    }
    for (dst = __psram_bss_start__; dst < __psram_bss_end__; dst++) {
        *dst = 0;
    }

    // jlink
#if 0
    hal_analogif_reg_write(0x6d,0x1701);
    REG(0x40000048) = 0x03ffffff;
    REG(0x4001f008) = 0xffffffff;
    REG(0x4001f020) = 0x00000000;
#endif
}

void BootInit(void)
{
    uint32_t *dst, *src;

    // Init icache
    hal_cache_enable(HAL_CACHE_ID_I_CACHE, HAL_CACHE_YES);

    // Init boot sections
    for (dst = __boot_sram_start__, src = __boot_sram_start_flash__;
            src < __boot_sram_end_flash__;
            dst++, src++) {
        *dst = *src;
    }

    for (dst = __boot_bss_sram_start__; dst < __boot_bss_sram_end__; dst++) {
        *dst = 0;
    }

#ifdef FPGA
    hal_cmu_fpga_setup();
#else
    hal_cmu_setup();
#endif

    // Init dcache and write buffer
    hal_cache_enable(HAL_CACHE_ID_D_CACHE, HAL_CACHE_YES);
    hal_cache_writebuffer_enable(HAL_CACHE_ID_D_CACHE, HAL_CACHE_YES);

    for (dst = __sram_text_data_start__, src = __sram_text_data_start_flash__;
            src < __sram_text_data_end_flash__;
            dst++, src++) {
        *dst = *src;
    }
    for (dst = __sram_bss_start__; dst < __sram_bss_end__; dst++) {
        *dst = 0;
    }
    for (dst = __fast_sram_text_data_start__, src = __fast_sram_text_data_start_flash__;
            src < __fast_sram_text_data_end_flash__;
            dst++, src++) {
        *dst = *src;
    }
}

// -----------------------------------------------------------
// Mutex flag
// -----------------------------------------------------------

int set_bool_flag(bool *flag)
{
    bool busy;

    do {
        busy = (bool)__LDREXB((unsigned char *)flag);
        if (busy) {
            __CLREX();
            return -1;
        }
    } while (__STREXB(true, (unsigned char *)flag));
    __DMB();

    return 0;
}

void clear_bool_flag(bool *flag)
{
    *flag = false;
    __DMB();
}

