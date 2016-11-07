#ifndef __REG_DMA_H__
#define __REG_DMA_H__

#include "plat_types.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Number of channels on GPDMA and AUDMA
 */
#define DMA_NUMBER_CHANNELS 8

/**
 * DMA Channel register block structure
 */
struct DMA_CH_T {
    __IO uint32_t  SRCADDR;                /* DMA Channel Source Address Register */
    __IO uint32_t  DSTADDR;                /* DMA Channel Destination Address Register */
    __IO uint32_t  LLI;                    /* DMA Channel Linked List Item Register */
    __IO uint32_t  CONTROL;                /* DMA Channel Control Register */
    __IO uint32_t  CONFIG;                /* DMA Channel Configuration Register */
    __I  uint32_t  RESERVED1[3];
};

/**
 * DMA register block
 */
struct DMA_T {                            /* DMA Structure */
    __I  uint32_t  INTSTAT;                /* DMA Interrupt Status Register */
    __I  uint32_t  INTTCSTAT;            /* DMA Interrupt Terminal Count Request Status Register */
    __O  uint32_t  INTTCCLR;            /* DMA Interrupt Terminal Count Request Clear Register */
    __I  uint32_t  INTERRSTAT;            /* DMA Interrupt Error Status Register */
    __O  uint32_t  INTERRCLR;            /* DMA Interrupt Error Clear Register */
    __I  uint32_t  RAWINTTCSTAT;        /* DMA Raw Interrupt Terminal Count Status Register */
    __I  uint32_t  RAWINTERRSTAT;        /* DMA Raw Error Interrupt Status Register */
    __I  uint32_t  ENBLDCHNS;            /* DMA Enabled Channel Register */
    __IO uint32_t  SOFTBREQ;            /* DMA Software Burst Request Register */
    __IO uint32_t  SOFTSREQ;            /* DMA Software Single Request Register */
    __IO uint32_t  SOFTLBREQ;            /* DMA Software Last Burst Request Register */
    __IO uint32_t  SOFTLSREQ;            /* DMA Software Last Single Request Register */
    __IO uint32_t  DMACONFIG;            /* DMA Configuration Register */
    __IO uint32_t  SYNC;                /* DMA Synchronization Register */
    __I  uint32_t  RESERVED0[50];
    struct DMA_CH_T CH[DMA_NUMBER_CHANNELS];
};

/**
 * Macro defines for DMA channel control registers
 */
#define MAX_TRANSFERSIZE              (0xFFF)

#define DMA_CONTROL_TRANSFERSIZE(n)   ((((n) & 0xFFF) << 0))    /* Transfer size*/
#define DMA_CONTROL_TRANSFERSIZE_MASK (0xFFF << 0)
#define DMA_CONTROL_TRANSFERSIZE_SHIFT (0)
#define DMA_CONTROL_SBSIZE(n)         ((((n) & 0x07) << 12))    /* Source burst size*/
#define DMA_CONTROL_DBSIZE(n)         ((((n) & 0x07) << 15))    /* Destination burst size*/
#define DMA_CONTROL_SWIDTH(n)         ((((n) & 0x07) << 18))    /* Source transfer width*/
#define DMA_CONTROL_SWIDTH_MASK       (0x07 << 18)
#define DMA_CONTROL_SWIDTH_SHIFT      (18)
#define DMA_CONTROL_DWIDTH(n)         ((((n) & 0x07) << 21))    /* Destination transfer width*/
#define DMA_CONTROL_SI                ((1UL << 26))            /* Source increment*/
#define DMA_CONTROL_DI                ((1UL << 27))            /* Destination increment*/
#define DMA_CONTROL_SRCAHB1           0
#define DMA_CONTROL_DSTAHB1           0
#define DMA_CONTROL_PROT1             ((1UL << 28))            /* Indicates that the access is in user mode or privileged mode*/
#define DMA_CONTROL_PROT2             ((1UL << 29))            /* Indicates that the access is bufferable or not bufferable*/
#define DMA_CONTROL_PROT3             ((1UL << 30))            /* Indicates that the access is cacheable or not cacheable*/
#define DMA_CONTROL_TC_IRQ            ((1UL << 31))            /* Terminal count interrupt enable bit */

/**
 * Macro defines for DMA Channel Configuration registers
 */
#define DMA_CONFIG_EN                 ((1UL << 0))            /* DMA control enable*/
#define DMA_CONFIG_SRCPERIPH(n)       ((((n) & 0x1F) << 1))    /* Source peripheral*/
#define DMA_CONFIG_DSTPERIPH(n)       ((((n) & 0x1F) << 6))    /* Destination peripheral*/
#define DMA_CONFIG_TRANSFERTYPE(n)    ((((n) & 0x7) << 11))    /* This value indicates the type of transfer*/
#define DMA_CONFIG_ERR_IRQMASK        ((1UL << 14))            /* Interrupt error mask*/
#define DMA_CONFIG_TC_IRQMASK         ((1UL << 15))            /* Terminal count interrupt mask*/
#define DMA_CONFIG_LOCK               ((1UL << 16))            /* Lock*/
#define DMA_CONFIG_ACTIVE             ((1UL << 17))            /* Active*/
#define DMA_CONFIG_HALT               ((1UL << 18))            /* Halt*/
#define DMA_CONFIG_TRY_BURST          ((1UL << 19))         /* Try burst*/

#define DMA_STAT_CHAN(n)              ((1 << (n)) & 0xFF)
#define DMA_STAT_CHAN_ALL             (0xFF)

/**
 * Macro defines for DMA Configuration register
 */
#define DMA_DMACONFIG_EN              ((0x01))    /* DMA Controller enable*/
#define DMA_DMACONFIG_AHB1_BIGENDIAN  ((0x02))    /* AHB Master endianness configuration*/
#define DMA_DMACONFIG_AHB2_BIGENDIAN  ((0x04))    /* AHB Master endianness configuration*/

#ifdef __cplusplus
}
#endif

#endif

