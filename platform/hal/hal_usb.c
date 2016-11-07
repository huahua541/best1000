#include "reg_usb.h"
#include "hal_usb.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "usb_descriptor.h"
#include "string.h"
#include "cmsis_nvic.h"
#include "pmu.h"

// TODO List:
// 1) IRQ priority
// 2) Thread-safe issue (race condition)

// Hardware configuration:
// GHWCFG1 = 0x00000000
// GHWCFG2 = 0x228a5512
// GHWCFG3 = 0x03f404e8
// GHWCFG4 = 0x16108020

#define USB_TRACE(mask, str, ...)           { if (usb_trmask & (1 << mask)) { TRACE(str, ##__VA_ARGS__); } }
#define USB_FUNC_ENTRY_TRACE(mask)          { if (usb_trmask & (1 << mask)) { FUNC_ENTRY_TRACE(); } }

enum DEVICE_STATE {
    ATTACHED,
    POWERED,
    DEFAULT,
    ADDRESS,
    CONFIGURED,
};

struct EPN_OUT_TRANSFER {
    uint8_t *data;
    uint32_t length;
    bool enabled;
};

struct EPN_IN_TRANSFER {
    const uint8_t *data;
    uint32_t length;
    uint16_t pkt_cnt;
    bool zero_len_pkt;
    bool enabled;
};

uint32_t usb_trmask = (1 << 0); //(1 << 2) | (1 << 6) | (1 << 8);

static struct USBC_T * const usbc = (struct USBC_T *)0x40180000;

static uint16_t fifo_addr;

static uint32_t ep0_out_buffer[EP0_MAX_PACKET_SIZE / 4];
static uint32_t ep0_in_buffer[EP0_MAX_PACKET_SIZE / 4];

static struct EPN_OUT_TRANSFER epn_out_transfer[EPNUM - 1];
static struct EPN_IN_TRANSFER epn_in_transfer[EPNUM - 1];

static struct EP0_TRANSFER ep0_transfer;

static struct HAL_USB_CALLBACKS callbacks;

static volatile enum DEVICE_STATE device_state = ATTACHED;
static uint8_t device_cfg = 0;
static uint8_t currentAlternate;
static uint16_t currentInterface;

static uint8_t device_suspended;
static uint8_t device_pwr_wkup_status;

static void hal_usb_irq_handler(void);

static void hal_usb_init_ep0_transfer(void)
{
    ep0_transfer.stage = NONE_STAGE;
    ep0_transfer.data = NULL;
    ep0_transfer.length = 0;
    ep0_transfer.trx_len = 0;
}

static void hal_usb_init_epn_transfer(void)
{
    memset(&epn_out_transfer[0], 0, sizeof(epn_out_transfer));
    memset(&epn_in_transfer[0], 0, sizeof(epn_in_transfer));
}

static void get_setup_packet(uint8_t *data, struct SETUP_PACKET *packet)
{
    packet->bmRequestType.direction = (data[0] & 0x80) >> 7;
    packet->bmRequestType.type = (data[0] & 0x60) >> 5;
    packet->bmRequestType.recipient = data[0] & 0x1f;
    packet->bRequest = data[1];
    packet->wValue = (data[2] | data[3] << 8);
    packet->wIndex = (data[4] | data[5] << 8);
    packet->wLength = (data[6] | data[7] << 8);
}

static void reset_epn_out_transfer(uint8_t ep)
{
    bool enabled;

    enabled = epn_out_transfer[ep - 1].enabled;

    // Clear epn_out_transfer[] before invoking the callback,
    // so that ep can be restarted in the callback
    memset(&epn_out_transfer[ep - 1], 0, sizeof(epn_out_transfer[0]));

    if (enabled && callbacks.epn_recv_compl[ep - 1]) {
        callbacks.epn_recv_compl[ep - 1](NULL, 0, XFER_COMPL_ERROR);
    }
}

static void reset_epn_in_transfer(uint8_t ep)
{
    bool enabled;

    enabled = epn_in_transfer[ep - 1].enabled;

    // Clear epn_in_transfer[] before invoking the callback,
    // so that ep can be restarted in the callback
    memset(&epn_in_transfer[ep - 1], 0, sizeof(epn_in_transfer[0]));

    if (enabled && callbacks.epn_send_compl[ep - 1]) {
        callbacks.epn_send_compl[ep - 1](NULL, 0, XFER_COMPL_ERROR);
    }
}

static void _disable_out_ep(uint8_t ep, uint32_t set, uint32_t clr)
{
    volatile uint32_t *ctrl;
    volatile uint32_t *intr;

    if (ep >= EPNUM) {
        return;
    }

    if (ep == 0) {
        // EP0 out cannot be disabled, but stalled
        if (set == USBC_STALL) {
            usbc->DOEPCTL0 |= USBC_STALL;
        }
        if (clr == USBC_STALL) {
            usbc->DOEPCTL0 |= USBC_SNAK;
            usbc->DOEPCTL0 &= ~USBC_STALL;
        }
        return;
    } else {
        ctrl = &usbc->DOEPnCONFIG[ep - 1].DOEPCTL;
        intr = &usbc->DOEPnCONFIG[ep - 1].DOEPINT;
    }

    if ((*ctrl & USBC_USBACTEP) == 0) {
        goto _exit;
    }
    if ((*ctrl & (USBC_EPENA | USBC_NAKSTS)) == USBC_NAKSTS) {
        goto _exit;
    }

    usbc->DCTL |= USBC_SGOUTNAK;
    while ((usbc->GINTSTS & USBC_GOUTNAKEFF) == 0);

    *ctrl |= USBC_EPENA | USBC_SNAK;
    // No status bit checking

    *ctrl |= USBC_EPDIS | set;
    if (clr) {
        *ctrl &= ~clr;
    }
    while ((*intr & USBC_EPDISBLD) == 0);

    usbc->DCTL &= ~USBC_SGOUTNAK;
    usbc->DCTL |= USBC_CGOUTNAK;

_exit:
    reset_epn_out_transfer(ep);
}

static void _disable_in_ep(uint8_t ep, uint32_t set, uint32_t clr)
{
    volatile uint32_t *ctrl;
    volatile uint32_t *intr;

    if (ep >= EPNUM) {
        return;
    }

    if (ep == 0) {
        ctrl = &usbc->DIEPCTL0;
        intr = &usbc->DIEPINT0;
    } else {
        ctrl = &usbc->DIEPnCONFIG[ep - 1].DIEPCTL;
        intr = &usbc->DIEPnCONFIG[ep - 1].DIEPINT;
    }

    if ((*ctrl & USBC_USBACTEP) == 0) {
        goto _exit;
    }
    if ((*ctrl & (USBC_EPENA | USBC_NAKSTS)) == USBC_NAKSTS) {
        goto _exit;
    }

    *ctrl |= USBC_EPENA | USBC_SNAK;
    while ((*intr & USBC_INEPNAKEFF) == 0);

    *ctrl |= USBC_EPDIS | set;
    if (clr) {
        *ctrl &= ~clr;
    }
    while ((*intr & USBC_EPDISBLD) == 0);

    usbc->GRSTCTL = USBC_TXFNUM(ep) | USBC_TXFFLSH;
    while ((usbc->GRSTCTL & USBC_TXFFLSH) != 0);

_exit:
    if (ep > 0) {
        reset_epn_in_transfer(ep);
    }
}

void hal_usb_disable_ep(enum EP_DIR dir, uint8_t ep)
{
    USB_TRACE(14, "%s: %d ep%d", __FUNCTION__, dir, ep);

    if (dir == EP_OUT) {
        _disable_out_ep(ep, USBC_SNAK, 0);
    } else {
        _disable_in_ep(ep, USBC_SNAK, 0);
    }
}

void hal_usb_stall_ep(enum EP_DIR dir, uint8_t ep)
{
    USB_TRACE(13, "%s: %d ep%d", __FUNCTION__, dir, ep);

    if (dir == EP_OUT) {
        _disable_out_ep(ep, USBC_STALL, 0);
    } else {
        _disable_in_ep(ep, USBC_STALL, 0);
    }
}

void hal_usb_unstall_ep(enum EP_DIR dir, uint8_t ep)
{
    USB_TRACE(12, "%s: %d ep%d", __FUNCTION__, dir, ep);

    if (dir == EP_OUT) {
        _disable_out_ep(ep, USBC_SNAK, (USBC_STALL | USBC_USBACTEP));
    } else {
        _disable_in_ep(ep, USBC_SNAK, (USBC_STALL | USBC_USBACTEP));
    }
}

int hal_usb_get_ep_stall_state(enum EP_DIR dir, uint8_t ep)
{
    volatile uint32_t *ctrl;

    if (ep >= EPNUM) {
        return 0;
    }

    // Select ctl register
    if(ep == 0)
    {
        if (dir == EP_IN) {
            ctrl = &usbc->DIEPCTL0;
        } else {
            ctrl = &usbc->DOEPCTL0;
        }
    }
    else
    {
        if (dir == EP_IN) {
            ctrl = &usbc->DIEPnCONFIG[ep - 1].DIEPCTL;
        } else {
            ctrl = &usbc->DOEPnCONFIG[ep - 1].DOEPCTL;
        }
    }

    return ((*ctrl & USBC_STALL) != 0);
}

void hal_usb_stop_ep(enum EP_DIR dir, uint8_t ep)
{
    USB_TRACE(11, "%s: %d ep%d", __FUNCTION__, dir, ep);

    hal_usb_disable_ep(dir, ep);
}

static void hal_usb_stop_all_out_eps(void)
{
    int i;

    USB_FUNC_ENTRY_TRACE(11);

    // EP0 out cannot be disabled
    for (i = 1; i < EPNUM; i++) {
        if ((usbc->DOEPnCONFIG[i - 1].DOEPCTL & USBC_USBACTEP) == 0) {
            continue;
        }
        usbc->DOEPnCONFIG[i - 1].DOEPCTL |= USBC_EPENA | USBC_SNAK;
    }

    // Enable global out nak
    if ((usbc->GINTMSK & USBC_GOUTNAKEFF) != 0) {
        usbc->GINTMSK &= ~USBC_GOUTNAKEFF;
    }
    usbc->DCTL |= USBC_SGOUTNAK;
    while ((usbc->GINTSTS & USBC_GOUTNAKEFF) == 0);

    for (i = 1; i < EPNUM; i++) {
        if ((usbc->DOEPnCONFIG[i - 1].DOEPCTL & USBC_USBACTEP) == 0) {
            continue;
        }
        usbc->DOEPnCONFIG[i - 1].DOEPCTL |= USBC_EPDIS;
        while ((usbc->DOEPnCONFIG[i - 1].DOEPINT & USBC_EPDISBLD) == 0);
    }

    // Disable global out nak
    usbc->DCTL &= ~USBC_SGOUTNAK;
    usbc->DCTL |= USBC_CGOUTNAK;
}

static void hal_usb_stop_all_in_eps(void)
{
    int i;

    USB_FUNC_ENTRY_TRACE(11);

    usbc->DCTL |= USBC_SGNPINNAK;
    while ((usbc->GINTSTS & USBC_GINNAKEFF) == 0);

    for (i = 0; i < EPNUM; i++) {
        if (i == 0) {
            // EP0 in is always activated
            usbc->DIEPCTL0 |= USBC_EPENA | USBC_SNAK;
            while ((usbc->DIEPINT0 & USBC_INEPNAKEFF) == 0);
            usbc->DIEPCTL0 |= USBC_EPDIS;
            while ((usbc->DIEPINT0 & USBC_EPDISBLD) == 0);
        } else {
            if ((usbc->DIEPnCONFIG[i - 1].DIEPCTL & USBC_USBACTEP) == 0) {
                continue;
            }
            usbc->DIEPnCONFIG[i - 1].DIEPCTL |= USBC_EPENA | USBC_SNAK;
            while ((usbc->DIEPnCONFIG[i - 1].DIEPINT & USBC_INEPNAKEFF) == 0);
            usbc->DIEPnCONFIG[i - 1].DIEPCTL |= USBC_EPDIS;
            while ((usbc->DIEPnCONFIG[i - 1].DIEPINT & USBC_EPDISBLD) == 0);
        }
    }

    // Flush Tx Fifo
    usbc->GRSTCTL = USBC_TXFNUM(0x10) | USBC_TXFFLSH | USBC_RXFFLSH;
    while ((usbc->GRSTCTL & (USBC_TXFFLSH | USBC_RXFFLSH)) != 0);

    usbc->DCTL |= USBC_CGNPINNAK;
}

static void hal_usb_flush_tx_fifo(uint8_t ep)
{
    usbc->DCTL |= USBC_SGNPINNAK;
    while ((usbc->GINTSTS & USBC_GINNAKEFF) == 0);

    while ((usbc->GRSTCTL & USBC_AHBIDLE) == 0);

    //while ((usbc->GRSTCTL & USBC_TXFFLSH) != 0);

    usbc->GRSTCTL = USBC_TXFNUM(ep) | USBC_TXFFLSH;
    while ((usbc->GRSTCTL & USBC_TXFFLSH) != 0);

    usbc->DCTL |= USBC_CGNPINNAK;
}

static void hal_usb_flush_all_tx_fifos(void)
{
    hal_usb_flush_tx_fifo(0x10);
}

static void POSSIBLY_UNUSED hal_usb_flush_rx_fifo(void)
{
    usbc->DCTL |= USBC_SGOUTNAK;
    while ((usbc->GINTSTS & USBC_GOUTNAKEFF) == 0);

    usbc->GRSTCTL |= USBC_RXFFLSH;
    while ((usbc->GRSTCTL & USBC_RXFFLSH) != 0);

    usbc->DCTL &= ~USBC_SGOUTNAK;
    usbc->DCTL |= USBC_CGOUTNAK;
}

static void hal_usb_alloc_ep0_fifo(void)
{
    int i;
    int stop_in = 0;
    int stop_out = 0;

    if ((usbc->GHWCFG2 & USBC_DYNFIFOSIZING) == 0) {
        return;
    }

    // Stop endpoints
    if ((usbc->DIEPCTL0 & (USBC_NAKSTS | USBC_EPENA)) != (USBC_NAKSTS)) {
        stop_in = 1;
    }
    if ((usbc->DOEPCTL0 & (USBC_NAKSTS | USBC_EPENA)) != (USBC_NAKSTS)) {
        stop_out = 1;
    }

    for (i = 1; i < EPNUM; i++) {
        if ((usbc->DIEPnCONFIG[i - 1].DIEPCTL & (USBC_NAKSTS | USBC_EPENA)) != (USBC_NAKSTS)) {
            stop_in = 1;
        }
        if ((usbc->DOEPnCONFIG[i - 1].DOEPCTL & (USBC_NAKSTS | USBC_EPENA)) != (USBC_NAKSTS)) {
            stop_out = 1;
        }
    }

    if (stop_in) {
        hal_usb_stop_all_in_eps();
    }
    if (stop_out) {
        hal_usb_stop_all_out_eps();
    }

    // Configure FIFOs
#ifdef USB_ISO
#define RXFIFOSIZE ((CTRL_EPNUM * 4) + 6 + 1 + (2 * (MAX_ISO_PACKET_SIZE / 4 + 1)) + (EPNUM * 1))
#else
#define RXFIFOSIZE ((CTRL_EPNUM * 4) + 6 + 1 + (2 * (MAX_PACKET_SIZE / 4 + 1)) + (EPNUM * 1))
#endif
#define TXFIFOSIZE (2 * (MAX_PACKET_SIZE + 3) / 4)

#if (RXFIFOSIZE + TXFIFOSIZE > SPFIFORAM_SIZE)
#error "Invalid FIFO size configuration"
#endif

    // Rx Fifo Size (and init fifo_addr)
    usbc->GRXFSIZ = USBC_RXFDEP(RXFIFOSIZE);
    fifo_addr = RXFIFOSIZE;

    // EP0 / Non-periodic Tx Fifo Size
    usbc->GNPTXFSIZ = USBC_NPTXFSTADDR(fifo_addr) | USBC_NPTXFDEPS(TXFIFOSIZE);
    fifo_addr += TXFIFOSIZE;

    // Flush Tx FIFOs
    hal_usb_flush_all_tx_fifos();
}

static void hal_usb_alloc_epn_fifo(uint8_t ep, uint16_t mps)
{
    uint16_t size;

    if (ep == 0 || ep >= EPNUM) {
        return;
    }

    size = 2 * (mps + 3) / 4;

    ASSERT(fifo_addr + size <= SPFIFORAM_SIZE,
        "Fifo overflow: fifo_addr=%d, size=%d, RAM=%d",
        fifo_addr, size, SPFIFORAM_SIZE);

    usbc->DTXFSIZE[ep - 1].DIEPTXFn = USBC_INEPNTXFSTADDR(fifo_addr) | USBC_INEPNTXFDEP(size);
    fifo_addr += size;
}

static void hal_usb_soft_reset(void)
{
    usbc->GRSTCTL |= USBC_CSFTRST;
    while ((usbc->GRSTCTL & USBC_CSFTRST) != 0);
    while ((usbc->GRSTCTL & USBC_AHBIDLE) == 0);
}

static void hal_usb_device_init(void)
{
    int i;

    hal_usb_soft_reset();

#if 1
    usbc->GUSBCFG |= USBC_FORCEDEVMODE | USBC_ULPIAUTORES | USBC_ULPIFSLS |
        USBC_PHYSEL | USBC_ULPI_UTMI_SEL;
    usbc->GUSBCFG &= ~(USBC_FSINTF | USBC_PHYIF | USBC_USBTRDTIM_MASK);
    usbc->GUSBCFG |= USBC_USBTRDTIM(15);
#else
    usbc->GUSBCFG |= USBC_FORCEDEVMODE | USBC_PHYSEL;
    usbc->GUSBCFG &= ~(USBC_FSINTF | USBC_PHYIF | USBC_USBTRDTIM_MASK);
    usbc->GUSBCFG |= USBC_USBTRDTIM(5);
#endif

    // Reset after setting the PHY parameters
    hal_usb_soft_reset();

    usbc->DCFG    &= ~(USBC_DEVSPD_MASK  | USBC_PERFRINT_MASK);
    // Configure FS USB 1.1 Phy
    usbc->DCFG    |= USBC_DEVSPD(3) | USBC_NZSTSOUTHSHK | USBC_PERFRINT(0);

    // Clear previous interrupts
    usbc->GINTMSK = 0;
    usbc->GINTSTS = ~0;
    usbc->DAINTMSK = 0;
    usbc->DOEPMSK = 0;
    usbc->DIEPMSK = 0;
    for (i = 0; i < EPNUM; ++i) {
        if (i == 0) {
            usbc->DOEPINT0 = ~0;
            usbc->DIEPINT0 = ~0;
        } else {
            usbc->DOEPnCONFIG[i - 1].DOEPINT = ~0;
            usbc->DIEPnCONFIG[i - 1].DIEPINT = ~0;
        }
    }
    usbc->GINTMSK = USBC_USBRST | USBC_ENUMDONE | USBC_ERLYSUSP | USBC_USBSUSP;

    usbc->DCTL &= ~USBC_SFTDISCON;

    // Enable DMA mode
    // Burst size 16 words
    usbc->GAHBCFG  = USBC_DMAEN | USBC_HBSTLEN(7);
    usbc->GAHBCFG |= USBC_GLBLINTRMSK;
}

static void hal_usb_soft_disconnect(void)
{
    // Disable global interrupt
    usbc->GAHBCFG &= ~USBC_GLBLINTRMSK;
    // Soft disconnection
    usbc->DCTL |= USBC_SFTDISCON;

    hal_usb_device_init();
}

static void enable_usb_irq(void)
{
    NVIC_SetVector(USB_IRQn, (uint32_t)hal_usb_irq_handler);
#ifdef USB_ISO
    NVIC_SetPriority(USB_IRQn, IRQ_PRIORITY_HIGH);
#else
    NVIC_SetPriority(USB_IRQn, IRQ_PRIORITY_NORMAL);
#endif
    NVIC_EnableIRQ(USB_IRQn);
}

int hal_usb_open(const struct HAL_USB_CALLBACKS *c, enum HAL_USB_API_MODE m)
{
    if (c == NULL) {
        return 1;
    }
    if (c->device_desc == NULL ||
        c->cfg_desc == NULL ||
        c->string_desc == NULL ||
        c->setcfg == NULL) {
        return 2;
    }

    if (device_state != ATTACHED) {
        return 3;
    }
    device_state = POWERED;
    device_suspended = 0;

    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, HAL_CMU_FREQ_52M);

    hal_cmu_usb_set_device_mode();
    hal_cmu_pll_enable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_USB);
    hal_cmu_clock_enable(HAL_CMU_MOD_H_USBC);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_USB);
    hal_cmu_reset_set(HAL_CMU_MOD_H_USBC);
    hal_cmu_reset_set(HAL_CMU_MOD_O_USB);
    hal_sys_timer_delay(2);
    hal_cmu_reset_clear(HAL_CMU_MOD_H_USBC);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_USB);

    memcpy(&callbacks, c, sizeof(callbacks));

    if (usbc->GAHBCFG & USBC_GLBLINTRMSK) {
        hal_usb_soft_disconnect();
    } else {
        hal_usb_device_init();
    }

    enable_usb_irq();

    if (m == HAL_USB_API_BLOCKING) {
        while (device_state != CONFIGURED);
    }

    return 0;
}

int hal_usb_reopen(const struct HAL_USB_CALLBACKS *c, uint8_t dcfg, uint8_t alt, uint16_t itf)
{
    if (c == NULL) {
        return 1;
    }
    if (c->device_desc == NULL ||
        c->cfg_desc == NULL ||
        c->string_desc == NULL ||
        c->setcfg == NULL) {
        return 2;
    }

    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, HAL_CMU_FREQ_52M);

    memcpy(&callbacks, c, sizeof(callbacks));

    device_state = CONFIGURED;
    device_cfg = dcfg;
    currentAlternate = alt;
    currentInterface = itf;

    enable_usb_irq();

    return 0;
}

void hal_usb_close(void)
{
    NVIC_DisableIRQ(USB_IRQn);

    device_state = ATTACHED;

    // Disable global interrupt
    usbc->GAHBCFG &= ~USBC_GLBLINTRMSK;
    // Soft disconnection
    usbc->DCTL |= USBC_SFTDISCON;
    // Soft reset
    usbc->GRSTCTL |= USBC_CSFTRST;
    usbc->DCTL |= USBC_SFTDISCON;
    usbc->GRSTCTL |= USBC_CSFTRST;
    //while ((usbc->GRSTCTL & USBC_CSFTRST) != 0);
    //while ((usbc->GRSTCTL & USBC_AHBIDLE) == 0);

    hal_cmu_reset_set(HAL_CMU_MOD_O_USB);
    hal_cmu_reset_set(HAL_CMU_MOD_H_USBC);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_USB);
    hal_cmu_clock_disable(HAL_CMU_MOD_H_USBC);
    hal_cmu_pll_disable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_USB);

    memset(&callbacks, 0, sizeof(callbacks));

    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, HAL_CMU_FREQ_32K);
}

int hal_usb_remote_wakeup(int signal)
{
#ifdef USB_SUSPEND
    USB_TRACE(15, "%s: %d", __FUNCTION__, signal);

    if (signal) {
        if (device_suspended == 0) {
            return 1;
        }
        if ((device_pwr_wkup_status & DEVICE_STATUS_REMOTE_WAKEUP) == 0) {
            return 2;
        }

        pmu_usb_disable_pin_status_check();

        hal_sysfreq_req(HAL_SYSFREQ_USER_USB, HAL_CMU_FREQ_52M);
        hal_cmu_clock_enable(HAL_CMU_MOD_H_USBC);
        hal_cmu_clock_enable(HAL_CMU_MOD_O_USB);

        usbc->DCTL |= USBC_RMTWKUPSIG;
    } else {
        usbc->DCTL &= ~USBC_RMTWKUPSIG;
    }
#endif

    return 0;
}

void hal_usb_detect_disconn(void)
{
    // NOTE:
    // PHY detects the disconnection event by DP/DN voltage level change.
    // But DP/DN voltages are provided by vusb ldo inside chip, which has nothing
    // to do with VBUS. That is why USB controller cannot generate the disconnection
    // interrupt.
    // Meanwhile VBUS detection or charger detection can help to generate USB
    // disconnection event via this function.

    USB_FUNC_ENTRY_TRACE(26);

    if (device_state != ATTACHED && callbacks.state_change) {
        callbacks.state_change(HAL_USB_EVENT_DISCONNECT);
    }
}

int hal_usb_configured(void)
{
    return (device_state == CONFIGURED);
}

int hal_usb_suspended(void)
{
    return device_suspended;
}

void hal_usb_activate_epn(enum EP_DIR dir, uint8_t ep, uint8_t type, uint16_t mps)
{
    USB_TRACE(10, "%s: %d ep%d", __FUNCTION__, dir, ep);

    if (ep == 0 || ep >= EPNUM) {
        return;
    }

    if (dir == EP_OUT) {
        // Stop ep out
        if ((usbc->DOEPnCONFIG[ep - 1].DOEPCTL & (USBC_NAKSTS | USBC_EPENA)) != (USBC_NAKSTS)) {
            hal_usb_stop_ep(dir, ep);
        }
        // Config ep out
        usbc->DOEPnCONFIG[ep - 1].DOEPCTL =
            USBC_EPN_MPS(mps) | USBC_EPTYPE(type) |
            USBC_USBACTEP | USBC_SNAK | USBC_SETD0PID;
        // Unmask ep out interrupt
        usbc->DAINTMSK |= USBC_OUTEPMSK(1 << ep);
    } else {
        // Stop ep in
        if ((usbc->DIEPnCONFIG[ep - 1].DIEPCTL & (USBC_NAKSTS | USBC_EPENA)) != (USBC_NAKSTS)) {
            hal_usb_stop_ep(dir, ep);
        }
        // Config ep in
        usbc->DIEPnCONFIG[ep - 1].DIEPCTL =
            USBC_EPN_MPS(mps) | USBC_EPTYPE(type) |
            USBC_USBACTEP | USBC_EPTXFNUM(ep) | USBC_SNAK | USBC_SETD0PID;
        // Allocate tx fifo
        hal_usb_alloc_epn_fifo(ep, mps);
        // Unmask ep in interrupt
        usbc->DAINTMSK |= USBC_INEPMSK(1 << ep);
    }
}

void hal_usb_deactivate_epn(enum EP_DIR dir, uint8_t ep)
{
    USB_TRACE(9, "%s: %d ep%d", __FUNCTION__, dir, ep);

    if (ep == 0 || ep >= EPNUM) {
        return;
    }

    hal_usb_stop_ep(dir, ep);

    if (dir == EP_OUT) {
        usbc->DOEPnCONFIG[ep - 1].DOEPCTL &= ~USBC_USBACTEP;
        // Mask ep out interrupt
        usbc->DAINTMSK &= ~ USBC_OUTEPMSK(1 << ep);
    } else {
        usbc->DIEPnCONFIG[ep - 1].DIEPCTL &= ~USBC_USBACTEP;
        // Mask ep in interrupt
        usbc->DAINTMSK &= ~ USBC_INEPMSK(1 << ep);
    }
}

static void hal_usb_recv_ep0(void)
{
    USB_FUNC_ENTRY_TRACE(8);

    // Enable EP0 to receive a new setup packet
    usbc->DOEPTSIZ0 = USBC_SUPCNT(3) | USBC_OEPXFERSIZE0(EP0_MAX_PACKET_SIZE) | USBC_OEPPKTCNT0;
    usbc->DOEPDMA0 =  (uint32_t)ep0_out_buffer;
    usbc->DOEPINT0 = usbc->DOEPINT0;
    usbc->DOEPCTL0 |= USBC_CNAK | USBC_EPENA;
}

static void hal_usb_send_ep0(const uint8_t *data, uint16_t size)
{
    USB_TRACE(8, "%s: %d", __FUNCTION__, size);
    ASSERT(size <= EP0_MAX_PACKET_SIZE, "Invalid ep0 send size: %d", size);

    if (data && size) {
        memcpy(ep0_in_buffer, data, size);
    }

    // Enable EP0 to send one packet
    usbc->DIEPTSIZ0 = USBC_IEPXFERSIZE0(size) | USBC_IEPPKTCNT0(1);
    usbc->DIEPDMA0 =  (uint32_t)ep0_in_buffer;
    usbc->DIEPINT0 = usbc->DIEPINT0;
    usbc->DIEPCTL0 |= USBC_CNAK | USBC_EPENA;
}

int hal_usb_recv_epn(uint8_t ep, uint8_t *buffer, uint32_t size)
{
    uint16_t mps;
    uint32_t pkt;
    uint32_t fn = 0;
#ifdef USB_ISO
    bool isoEp;
    uint32_t lock;
#endif

    USB_TRACE(7, "%s: ep%d %d", __FUNCTION__, ep, size);

    if (device_state != CONFIGURED) {
        return 1;
    }
    if (ep == 0 || ep > EPNUM) {
        return 2;
    }
    if (((uint32_t)buffer & 0x3) != 0) {
        return 3;
    }
    if (epn_out_transfer[ep - 1].data != NULL) {
        return 4;
    }
    if ((usbc->DOEPnCONFIG[ep - 1].DOEPCTL & USBC_USBACTEP) == 0) {
        return 5;
    }
    mps = GET_BITFIELD(usbc->DOEPnCONFIG[ep - 1].DOEPCTL, USBC_EPN_MPS);
    if (size < mps) {
        return 6;
    }

    if (size > EPN_MAX_XFERSIZE) {
        size = EPN_MAX_XFERSIZE;
    }
    pkt = size / mps;
    if (pkt > EPN_MAX_PKTCNT) {
        pkt = EPN_MAX_PKTCNT;
    }
    size = pkt * mps;

    epn_out_transfer[ep - 1].data = buffer;
    epn_out_transfer[ep - 1].length = size;
    epn_out_transfer[ep - 1].enabled = true;

    usbc->DOEPnCONFIG[ep - 1].DOEPTSIZ = USBC_OEPXFERSIZE(size) | USBC_OEPPKTCNT(pkt);
    usbc->DOEPnCONFIG[ep - 1].DOEPDMA = (uint32_t)buffer;
    usbc->DOEPnCONFIG[ep - 1].DOEPINT = usbc->DOEPnCONFIG[ep - 1].DOEPINT;

#ifdef USB_ISO
    if (GET_BITFIELD(usbc->DOEPnCONFIG[ep - 1].DOEPCTL, USBC_EPTYPE) == E_ISOCHRONOUS) {
        isoEp = true;
    } else {
        isoEp = false;
    }
    if (isoEp) {
        // Set the frame number in time
        lock = int_lock();
        // Get next frame number
        if (GET_BITFIELD(usbc->DSTS, USBC_SOFFN) & 0x1) {
            fn = USBC_SETD0PID;
        } else {
            fn = USBC_SETD1PID;
        }
    }
#endif

    usbc->DOEPnCONFIG[ep - 1].DOEPCTL |= USBC_EPENA | USBC_CNAK | fn;

#ifdef USB_ISO
    if (isoEp) {
        int_unlock(lock);
    }
#endif

    return 0;
}

int hal_usb_send_epn(uint8_t ep, const uint8_t *buffer, uint32_t size, enum ZLP_STATE zlp)
{
    uint16_t mps;
    uint8_t type;
    uint32_t pkt;
    uint32_t fn = 0;
#ifdef USB_ISO
    bool isoEp;
    uint32_t lock;
#endif

    USB_TRACE(6, "%s: ep%d %d", __FUNCTION__, ep, size);

    if (device_state != CONFIGURED) {
        return 1;
    }
    if (ep == 0 || ep > EPNUM) {
        return 2;
    }
    if (((uint32_t)buffer & 0x3) != 0) {
        return 3;
    }
    if (epn_in_transfer[ep - 1].data != NULL) {
        return 4;
    }
    if ((usbc->DIEPnCONFIG[ep - 1].DIEPCTL & USBC_USBACTEP) == 0) {
        return 5;
    }
    if (size > EPN_MAX_XFERSIZE) {
        return 7;
    }
    mps = GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPCTL, USBC_EPN_MPS);
    if (size <= mps) {
        // Also taking care of 0 size packet
        pkt = 1;
    } else {
        pkt = (size + mps - 1) / mps;
        if (pkt > EPN_MAX_PKTCNT) {
            return 8;
        }
    }

    epn_in_transfer[ep - 1].data = buffer;
    epn_in_transfer[ep - 1].length = size;
    epn_in_transfer[ep - 1].pkt_cnt = pkt;
    epn_in_transfer[ep - 1].enabled = true;

    type = GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPCTL, USBC_EPTYPE);
    if (type == E_INTERRUPT || type == E_ISOCHRONOUS) {
        // Never send a zero length packet at the end of transfer
        epn_in_transfer[ep - 1].zero_len_pkt = false;
        usbc->DIEPnCONFIG[ep - 1].DIEPTSIZ = USBC_IEPXFERSIZE(size) | USBC_IEPPKTCNT(pkt) | USBC_MC(1);
    } else {
        // Check if a zero length packet is needed at the end of transfer
        if (zlp == ZLP_AUTO) {
            epn_in_transfer[ep - 1].zero_len_pkt = ((size % mps) == 0);
        } else {
            epn_in_transfer[ep - 1].zero_len_pkt = false;
        }
        usbc->DIEPnCONFIG[ep - 1].DIEPTSIZ = USBC_IEPXFERSIZE(size) | USBC_IEPPKTCNT(pkt);
    }
    usbc->DIEPnCONFIG[ep - 1].DIEPDMA = (uint32_t)buffer;
    usbc->DIEPnCONFIG[ep - 1].DIEPINT = usbc->DIEPnCONFIG[ep - 1].DIEPINT;

#ifdef USB_ISO
    if (GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPCTL, USBC_EPTYPE) == E_ISOCHRONOUS) {
        isoEp = true;
    } else {
        isoEp = false;
    }
    if (isoEp) {
        // Set the frame number in time
        lock = int_lock();
        // Get next frame number
        if (GET_BITFIELD(usbc->DSTS, USBC_SOFFN) & 0x1) {
            fn = USBC_SETD0PID;
        } else {
            fn = USBC_SETD1PID;
        }
    }
#endif

    usbc->DIEPnCONFIG[ep - 1].DIEPCTL |= USBC_EPENA | USBC_CNAK | fn;

#ifdef USB_ISO
    if (isoEp) {
        int_unlock(lock);
    }
#endif

    return 0;
}

static void hal_usb_recv_epn_complete(uint8_t ep, uint32_t statusEp)
{
    uint8_t *data;
    uint32_t size;

    USB_TRACE(5, "%s: ep%d 0x%08x", __FUNCTION__, ep, statusEp);

    if (!epn_out_transfer[ep - 1].enabled) {
        return;
    }

    data = epn_out_transfer[ep - 1].data;
    size = GET_BITFIELD(usbc->DOEPnCONFIG[ep - 1].DOEPTSIZ, USBC_OEPXFERSIZE);
    ASSERT(size <= epn_out_transfer[ep - 1].length,
        "Invalid xfer size: size=%d, len=%d", size, epn_out_transfer[ep - 1].length);
    size = epn_out_transfer[ep - 1].length - size;

    // Clear epn_out_transfer[] before invoking the callback,
    // so that ep can be restarted in the callback
    memset(&epn_out_transfer[ep - 1], 0, sizeof(epn_out_transfer[0]));

    if (callbacks.epn_recv_compl[ep - 1]) {
        callbacks.epn_recv_compl[ep - 1](data, size, XFER_COMPL_SUCCESS);
    }
}

static void hal_usb_send_epn_complete(uint8_t ep, uint32_t statusEp)
{
    const uint8_t *data;
    uint32_t size;
    uint16_t pkt, mps;
    enum XFER_COMPL_STATE state = XFER_COMPL_SUCCESS;

    USB_TRACE(4, "%s: ep%d 0x%08x", __FUNCTION__, ep, statusEp);

    if (!epn_in_transfer[ep - 1].enabled) {
        return;
    }

    if ((statusEp & USBC_XFERCOMPLMSK) == 0) {
        state = XFER_COMPL_ERROR;
    }

    pkt = GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPTSIZ, USBC_IEPPKTCNT);
    if (pkt != 0) {
        state = XFER_COMPL_ERROR;
        mps = GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPCTL, USBC_EPN_MPS);
        ASSERT(pkt <= epn_in_transfer[ep - 1].pkt_cnt, "Invalid pkt cnt: pkt=%d, pkt_cnt=%d",
            pkt, epn_in_transfer[ep - 1].pkt_cnt);
        size = (epn_in_transfer[ep - 1].pkt_cnt - pkt) * mps;
    } else {
        size = epn_in_transfer[ep - 1].length;
    }

    if (state == XFER_COMPL_SUCCESS && epn_in_transfer[ep - 1].zero_len_pkt) {
        epn_in_transfer[ep - 1].zero_len_pkt = false;
        // Send the last zero length packet, except for isochronous/interrupt endpoints
        usbc->DIEPnCONFIG[ep - 1].DIEPTSIZ = USBC_IEPXFERSIZE(0) | USBC_IEPPKTCNT(1);
        usbc->DIEPnCONFIG[ep - 1].DIEPCTL |= USBC_EPENA | USBC_CNAK;
    } else {
        data = epn_in_transfer[ep - 1].data;

        // Clear epn_in_transfer[] before invoking the callback,
        // so that ep can be restarted in the callback
        memset(&epn_in_transfer[ep - 1], 0, sizeof(epn_in_transfer[0]));

        if (state != XFER_COMPL_SUCCESS) {
            // The callback will not be invoked when stopping ep,
            // for epn_in_transfer[] has been cleared
            hal_usb_stop_ep(EP_IN, ep);
        }

        if (callbacks.epn_send_compl[ep - 1]) {
            callbacks.epn_send_compl[ep - 1](data, size, state);
        }
    }

}

static bool requestSetAddress(void)
{
    USB_FUNC_ENTRY_TRACE(25);

    /* Set the device address */
    usbc->DCFG = SET_BITFIELD(usbc->DCFG, USBC_DEVADDR, ep0_transfer.setup_pkt.wValue);
    ep0_transfer.stage = STATUS_IN_STAGE;

    if (ep0_transfer.setup_pkt.wValue == 0)
    {
        device_state = DEFAULT;
    }
    else
    {
        device_state = ADDRESS;
    }

    return true;
}

static bool requestSetConfiguration(void)
{
    USB_FUNC_ENTRY_TRACE(24);

    device_cfg = ep0_transfer.setup_pkt.wValue;
    /* Set the device configuration */
    if (device_cfg == 0)
    {
        /* Not configured */
        device_state = ADDRESS;
    }
    else
    {
        if (callbacks.setcfg && callbacks.setcfg(device_cfg))
        {
            /* Valid configuration */
            device_state = CONFIGURED;
            ep0_transfer.stage = STATUS_IN_STAGE;
        }
        else
        {
            return false;
        }
    }

    return true;
}

static bool requestGetConfiguration(void)
{
    USB_FUNC_ENTRY_TRACE(23);

    /* Send the device configuration */
    ep0_transfer.data = &device_cfg;
    ep0_transfer.length = sizeof(device_cfg);
    ep0_transfer.stage = DATA_IN_STAGE;
    return true;
}

static bool requestGetInterface(void)
{
    USB_FUNC_ENTRY_TRACE(22);

    /* Return the selected alternate setting for an interface */
    if (device_state != CONFIGURED)
    {
        return false;
    }

    /* Send the alternate setting */
    ep0_transfer.setup_pkt.wIndex = currentInterface;
    ep0_transfer.data = &currentAlternate;
    ep0_transfer.length = sizeof(currentAlternate);
    ep0_transfer.stage = DATA_IN_STAGE;
    return true;
}

static bool requestSetInterface(void)
{
    bool success = false;

    USB_FUNC_ENTRY_TRACE(21);

    if(callbacks.setitf && callbacks.setitf(ep0_transfer.setup_pkt.wIndex, ep0_transfer.setup_pkt.wValue))
    {
        success = true;
        currentInterface = ep0_transfer.setup_pkt.wIndex;
        currentAlternate = ep0_transfer.setup_pkt.wValue;
        ep0_transfer.stage = STATUS_IN_STAGE;
    }
    return success;
}

static bool requestSetFeature(void)
{
    bool success = false;

    USB_FUNC_ENTRY_TRACE(20);

    if (device_state != CONFIGURED)
    {
        /* Endpoint or interface must be zero */
        if (ep0_transfer.setup_pkt.wIndex != 0)
        {
            return false;
        }
    }

    switch (ep0_transfer.setup_pkt.bmRequestType.recipient)
    {
        case DEVICE_RECIPIENT:
#ifdef USB_SUSPEND
            if (ep0_transfer.setup_pkt.wValue == DEVICE_REMOTE_WAKEUP) {
                if (callbacks.set_remote_wakeup) {
                    callbacks.set_remote_wakeup(1);
                    device_pwr_wkup_status |= DEVICE_STATUS_REMOTE_WAKEUP;
                    success = true;
                }
            }
#endif
            break;
        case ENDPOINT_RECIPIENT:
            if (ep0_transfer.setup_pkt.wValue == ENDPOINT_HALT)
            {
                /* TODO: We should check that the endpoint number is valid */
                hal_usb_stall_ep((ep0_transfer.setup_pkt.wIndex & 0x80) ? EP_IN : EP_OUT,
                    ep0_transfer.setup_pkt.wIndex & 0xF);
                success = true;
            }
            break;
        default:
            break;
    }

    ep0_transfer.stage = STATUS_IN_STAGE;

    return success;
}

static bool requestClearFeature(void)
{
    bool success = false;

    USB_FUNC_ENTRY_TRACE(19);

    if (device_state != CONFIGURED)
    {
        /* Endpoint or interface must be zero */
        if (ep0_transfer.setup_pkt.wIndex != 0)
        {
            return false;
        }
    }

    switch (ep0_transfer.setup_pkt.bmRequestType.recipient)
    {
        case DEVICE_RECIPIENT:
#ifdef USB_SUSPEND
            if (ep0_transfer.setup_pkt.wValue == DEVICE_REMOTE_WAKEUP) {
                if (callbacks.set_remote_wakeup) {
                    callbacks.set_remote_wakeup(0);
                    device_pwr_wkup_status &= ~DEVICE_STATUS_REMOTE_WAKEUP;
                    success = true;
                }
            }
#endif
            break;
        case ENDPOINT_RECIPIENT:
            /* TODO: We should check that the endpoint number is valid */
            if (ep0_transfer.setup_pkt.wValue == ENDPOINT_HALT)
            {
                hal_usb_unstall_ep((ep0_transfer.setup_pkt.wIndex & 0x80) ? EP_IN : EP_OUT,
                    ep0_transfer.setup_pkt.wIndex & 0xF);
                success = true;
            }
            break;
        default:
            break;
    }

    ep0_transfer.stage = STATUS_IN_STAGE;

    return success;
}

static bool requestGetStatus(void)
{
    static uint16_t status;
    bool success = false;

    USB_FUNC_ENTRY_TRACE(18);

    if (device_state != CONFIGURED)
    {
        /* Endpoint or interface must be zero */
        if (ep0_transfer.setup_pkt.wIndex != 0)
        {
            return false;
        }
    }

    switch (ep0_transfer.setup_pkt.bmRequestType.recipient)
    {
        case DEVICE_RECIPIENT:
            status = device_pwr_wkup_status;
            success = true;
            break;
        case INTERFACE_RECIPIENT:
            status = 0;
            success = true;
            break;
        case ENDPOINT_RECIPIENT:
            /* TODO: We should check that the endpoint number is valid */
            if (hal_usb_get_ep_stall_state((ep0_transfer.setup_pkt.wIndex & 0x80) ? EP_IN : EP_OUT,
                    ep0_transfer.setup_pkt.wIndex & 0xF))
            {
                status = ENDPOINT_STATUS_HALT;
            }
            else
            {
                status = 0;
            }
            success = true;
            break;
        default:
            break;
    }

    if (success)
    {
        /* Send the status */
        ep0_transfer.data = (uint8_t *)&status; /* Assumes little endian */
        ep0_transfer.length = sizeof(status);
        ep0_transfer.stage = DATA_IN_STAGE;
    }

    return success;
}

static bool requestGetDescriptor(void)
{
    bool success = false;
    uint8_t type = DESCRIPTOR_TYPE(ep0_transfer.setup_pkt.wValue);
    uint8_t index;
    uint8_t *desc;

    USB_TRACE(3, "%s: %d", __FUNCTION__, type);

    switch (type)
    {
        case DEVICE_DESCRIPTOR:
            desc = (uint8_t *)callbacks.device_desc(0);
            if (desc != NULL)
            {
                if ((desc[0] == DEVICE_DESCRIPTOR_LENGTH) \
                    && (desc[1] == DEVICE_DESCRIPTOR))
                {
                    ep0_transfer.length = DEVICE_DESCRIPTOR_LENGTH;
                    ep0_transfer.data = desc;
                    success = true;
                }
            }
            break;
        case CONFIGURATION_DESCRIPTOR:
            desc = (uint8_t *)callbacks.cfg_desc(0);
            if (desc != NULL)
            {
                if ((desc[0] == CONFIGURATION_DESCRIPTOR_LENGTH) \
                    && (desc[1] == CONFIGURATION_DESCRIPTOR))
                {
                    /* Get wTotalLength */
                    ep0_transfer.length = desc[2] \
                        | (desc[3] << 8);
                    ep0_transfer.data = desc;
                    // Get self-powered status
                    if ((desc[7] >> 6) & 0x01) {
                        device_pwr_wkup_status |= DEVICE_STATUS_SELF_POWERED;
                    } else {
                        device_pwr_wkup_status &= ~DEVICE_STATUS_SELF_POWERED;
                    }
                    success = true;
                }
            }
            break;
        case STRING_DESCRIPTOR:
            index = DESCRIPTOR_INDEX(ep0_transfer.setup_pkt.wValue);
            ep0_transfer.data = (uint8_t *)callbacks.string_desc(index);
            if (ep0_transfer.data) {
                ep0_transfer.length = ep0_transfer.data[0];
                success = true;
            }
            break;
        case INTERFACE_DESCRIPTOR:
        case ENDPOINT_DESCRIPTOR:
            /* TODO: Support is optional, not implemented here */
            break;
        case QUALIFIER_DESCRIPTOR:
            // Not supported in USB 1.1
            break;
        default:
            // Might be a class or vendor specific descriptor, which
            // should be handled in setuprecv callback
            ASSERT(false, "Unknown desc type: %d", type);
            break;
    }

    if (success) {
        if (ep0_transfer.length > ep0_transfer.setup_pkt.wLength) {
            ep0_transfer.length = ep0_transfer.setup_pkt.wLength;
        }
        ep0_transfer.stage = DATA_IN_STAGE;
    }

    return success;
}

static void hal_usb_reset_all_transfers(void)
{
    int ep;

    USB_FUNC_ENTRY_TRACE(31);

    for (ep = 1; ep < EPNUM; ep++) {
        reset_epn_out_transfer(ep);
        reset_epn_in_transfer(ep);
    }
}

static bool hal_usb_handle_setup(void)
{
    bool success = false;
    enum DEVICE_STATE old_state;

    USB_FUNC_ENTRY_TRACE(17);

    old_state = device_state;

    /* Process standard requests */
    if (ep0_transfer.setup_pkt.bmRequestType.type == STANDARD_TYPE)
    {
        switch (ep0_transfer.setup_pkt.bRequest)
        {
             case GET_STATUS:
                 success = requestGetStatus();
                 break;
             case CLEAR_FEATURE:
                 success = requestClearFeature();
                 break;
             case SET_FEATURE:
                 success = requestSetFeature();
                 break;
             case SET_ADDRESS:
                success = requestSetAddress();
                 break;
             case GET_DESCRIPTOR:
                 success = requestGetDescriptor();
                 break;
             case SET_DESCRIPTOR:
                 /* TODO: Support is optional, not implemented here */
                 success = false;
                 break;
             case GET_CONFIGURATION:
                 success = requestGetConfiguration();
                 break;
             case SET_CONFIGURATION:
                 success = requestSetConfiguration();
                 break;
             case GET_INTERFACE:
                 success = requestGetInterface();
                 break;
             case SET_INTERFACE:
                 success = requestSetInterface();
                 break;
             default:
                 break;
        }
    }

    if (old_state == CONFIGURED && device_state != CONFIGURED) {
        hal_usb_reset_all_transfers();
    }

    return success;
}

static int hal_usb_ep0_setup_stage(uint32_t statusEp)
{
    uint8_t *data;
    uint8_t setup_cnt;
    uint16_t pkt_len;

    if (statusEp & USBC_BACK2BACKSETUP) {
        data = (uint8_t *)(usbc->DOEPDMA0 - 8);
    } else {
        setup_cnt = GET_BITFIELD(usbc->DOEPTSIZ0, USBC_SUPCNT);
        if (setup_cnt >= 3) {
            setup_cnt = 2;
        }
        data = (uint8_t *)((uint32_t)ep0_out_buffer + 8 * (2 - setup_cnt));
    }
    // Init new transfer
    hal_usb_init_ep0_transfer();
    ep0_transfer.stage = SETUP_STAGE;
    get_setup_packet(data, &ep0_transfer.setup_pkt);

    USB_TRACE(2, "Got SETUP type=%d, req=0x%x, val=0x%x, idx=0x%x, len=%d",
        ep0_transfer.setup_pkt.bmRequestType.type,
        ep0_transfer.setup_pkt.bRequest,
        ep0_transfer.setup_pkt.wValue,
        ep0_transfer.setup_pkt.wIndex,
        ep0_transfer.setup_pkt.wLength);

    // Skip the check on IN/OUT tokens
    usbc->DOEPMSK &= ~USBC_OUTTKNEPDISMSK;
    usbc->DIEPMSK &= ~USBC_INTKNTXFEMPMSK;

    if (callbacks.setuprecv == NULL || !callbacks.setuprecv(&ep0_transfer)) {
        if (!hal_usb_handle_setup()) {
            return 1;
        }
    }

    if (ep0_transfer.setup_pkt.wLength == 0) {
        ep0_transfer.setup_pkt.bmRequestType.direction = EP_OUT;
    }

    if (ep0_transfer.stage == DATA_OUT_STAGE) {
        if (ep0_transfer.data == NULL) {
            ep0_transfer.data = (uint8_t *)ep0_out_buffer;
        }
    } else if (ep0_transfer.stage == DATA_IN_STAGE) {
        pkt_len = ep0_transfer.length;
        if (pkt_len > EP0_MAX_PACKET_SIZE) {
            pkt_len = EP0_MAX_PACKET_SIZE;
        }
        hal_usb_send_ep0(ep0_transfer.data, pkt_len);
    } else if(ep0_transfer.stage == STATUS_IN_STAGE) {
        hal_usb_send_ep0(NULL, 0);
    } else {
        USB_TRACE(0, "*** Setup stage switches to invalid stage: %d", ep0_transfer.stage);
        return 1;
    }

    return 0;
}

static int hal_usb_ep0_data_out_stage(void)
{
    uint16_t pkt_len;
    uint16_t reg_size;

    reg_size = GET_BITFIELD(usbc->DOEPTSIZ0, USBC_OEPXFERSIZE0);

    ASSERT(reg_size <= EP0_MAX_PACKET_SIZE, "Invalid recv size");

    pkt_len = MIN(EP0_MAX_PACKET_SIZE - reg_size, (uint16_t)(ep0_transfer.length - ep0_transfer.trx_len));
    memcpy(ep0_transfer.data + ep0_transfer.trx_len, ep0_out_buffer, pkt_len);
    ep0_transfer.trx_len += pkt_len;

    pkt_len = ep0_transfer.length - ep0_transfer.trx_len;
    if (pkt_len == 0) {
        if (callbacks.datarecv == NULL || !callbacks.datarecv(&ep0_transfer)) {
            //hal_usb_stall_ep(EP_OUT, 0);
            //hal_usb_stall_ep(EP_IN, 0);
            //return;
        }
        ep0_transfer.stage = STATUS_IN_STAGE;
        // Check error on IN/OUT tokens
        usbc->DOEPMSK |= USBC_OUTTKNEPDISMSK;
        // Send status packet
        hal_usb_send_ep0(NULL, 0);
    } else {
        // Receive next data packet
    }

    return 0;
}

static int hal_usb_ep0_data_in_stage(void)
{
    uint16_t pkt_len;
    bool zero_len_pkt = false;

    ASSERT(GET_BITFIELD(usbc->DIEPTSIZ0, USBC_IEPPKTCNT0) == 0, "Invalid sent pkt cnt");

    pkt_len = ep0_transfer.length - ep0_transfer.trx_len;
    if (pkt_len == 0) {
        // The last zero length packet was sent successfully
        ep0_transfer.stage = STATUS_OUT_STAGE;
        // Receive status packet (receiving is always enabled)
    } else {
        // Update sent count
        if (pkt_len == EP0_MAX_PACKET_SIZE) {
            zero_len_pkt = true;
        } else if (pkt_len > EP0_MAX_PACKET_SIZE) {
            pkt_len = EP0_MAX_PACKET_SIZE;
        }
        ep0_transfer.trx_len += pkt_len;

        // Send next packet
        pkt_len = ep0_transfer.length - ep0_transfer.trx_len;
        if (pkt_len > EP0_MAX_PACKET_SIZE) {
            pkt_len = EP0_MAX_PACKET_SIZE;
        }
        if (pkt_len == 0) {
            if (zero_len_pkt) {
                // Send the last zero length packet
                hal_usb_send_ep0(NULL, 0);
            } else {
                ep0_transfer.stage = STATUS_OUT_STAGE;
                // Check error on IN/OUT tokens
                usbc->DIEPMSK |= USBC_INTKNTXFEMPMSK;
                // Receive status packet (receiving is always enabled)
            }
        } else {
            hal_usb_send_ep0(ep0_transfer.data + ep0_transfer.trx_len, pkt_len);
        }
    }

    return 0;
}

static int hal_usb_ep0_status_stage(void)
{
    // Done with status packet
    ep0_transfer.stage = NONE_STAGE;
    // Skip the check on IN/OUT tokens
    usbc->DOEPMSK &= ~USBC_OUTTKNEPDISMSK;
    usbc->DIEPMSK &= ~USBC_INTKNTXFEMPMSK;

    return 0;
}

static void hal_usb_handle_ep0_packet(enum EP_DIR dir, uint32_t statusEp)
{
    int ret;

    USB_TRACE(16, "%s: dir=%d, statusEp=0x%08x", __FUNCTION__, dir, statusEp);

    if (dir == EP_OUT) {
        if ((statusEp & (USBC_XFERCOMPL | USBC_STSPHSERCVD)) == (USBC_XFERCOMPL | USBC_STSPHSERCVD)) {
            // From Linux driver
            if (GET_BITFIELD(usbc->DOEPTSIZ0, USBC_OEPXFERSIZE0) == EP0_MAX_PACKET_SIZE &&
                    (usbc->DOEPTSIZ0 & USBC_OEPPKTCNT0)) {
                // Abnormal case
                USB_TRACE(0, "*** EP0 OUT empty compl with stsphsercvd: stage=%d, size=0x%08x",
                    ep0_transfer.stage, usbc->DOEPTSIZ0);
                // Always enable setup packet receiving
                hal_usb_recv_ep0();
                return;
            }
        }

        if (statusEp & USBC_XFERCOMPL) {
            // New packet received
            if (statusEp & USBC_SETUP) {
                // Clean previous transfer
                if (ep0_transfer.stage != NONE_STAGE) {
                    USB_TRACE(0, "*** Setup stage breaks previous stage %d", ep0_transfer.stage);
                    hal_usb_stop_ep(EP_IN, 0);
                }
                // New setup packet
                ep0_transfer.stage = SETUP_STAGE;
            } else if (statusEp & USBC_STUPPKTRCVD) {
                // Clean previous transfer
                if (ep0_transfer.stage != NONE_STAGE) {
                    USB_TRACE(0, "*** Wait setup stage breaks previous stage %d", ep0_transfer.stage);
                    hal_usb_stop_ep(EP_IN, 0);
                }
                // New setup packet received, and wait for USBC h/w state machine finished
                // CAUTION: Enabling data receipt before getting USBC_SETUP interrupt might cause
                //          race condition in h/w, which leads to missing USBC_XFERCOMPL interrupt
                //          after the data has been received and acked
                ep0_transfer.stage = WAIT_SETUP_STAGE;
                return;
            }
        } else {
            // No packet received
            if (statusEp & USBC_SETUP) {
                if (ep0_transfer.stage == WAIT_SETUP_STAGE) {
                    // Previous packet is setup packet
                    ep0_transfer.stage = SETUP_STAGE;
                } else {
                    USB_TRACE(0, "*** Setup interrupt occurs in stage %d", ep0_transfer.stage);
                    // The setup packet has been processed
                    return;
                }
            }
        }
    }

    if (ep0_transfer.stage == SETUP_STAGE) {
        ASSERT(dir == EP_OUT, "Invalid dir for setup stage");

        ret = hal_usb_ep0_setup_stage(statusEp);
        if (ret) {
            // Error
            ep0_transfer.stage = NONE_STAGE;
            hal_usb_stall_ep(EP_OUT, 0);
            hal_usb_stall_ep(EP_IN, 0);
            // H/w will unstall EP0 automatically when a setup token is received
            hal_usb_recv_ep0();
        }
        // Always enable setup packet receiving
        hal_usb_recv_ep0();

        return;
    }

    if (statusEp & USBC_XFERCOMPL) {
        if (dir == EP_OUT) {
            if (ep0_transfer.stage == DATA_OUT_STAGE) {
                hal_usb_ep0_data_out_stage();
            } else if (ep0_transfer.stage == STATUS_OUT_STAGE) {
                hal_usb_ep0_status_stage();
            } else {
                // Abnormal case
                USB_TRACE(0, "*** EP0 OUT compl in stage %d with size 0x%08x", ep0_transfer.stage, usbc->DOEPTSIZ0);
            }
            // Always enable setup packet receiving
            hal_usb_recv_ep0();
        } else {
            if (ep0_transfer.stage == DATA_IN_STAGE) {
                hal_usb_ep0_data_in_stage();
            } else if (ep0_transfer.stage == STATUS_IN_STAGE) {
                hal_usb_ep0_status_stage();
            } else {
                // Abnormal case
                USB_TRACE(0, "*** EP0 IN compl in stage %d with size 0x%08x", ep0_transfer.stage, usbc->DIEPTSIZ0);
            }
        }
    }
}

static void hal_usb_irq_reset(void)
{
    int i;

    USB_FUNC_ENTRY_TRACE(2);

    device_state = DEFAULT;
    device_pwr_wkup_status = 0;
    device_suspended = 0;

    usbc->DCTL &= ~USBC_RMTWKUPSIG;

    hal_usb_reset_all_transfers();
    hal_usb_stop_all_out_eps();
    hal_usb_stop_all_in_eps();

    // Set NAK for all out endpoints
    usbc->DOEPCTL0 |= USBC_SNAK;
    for (i = 1; i < EPNUM; i++) {
        usbc->DOEPnCONFIG[i - 1].DOEPCTL |= USBC_SNAK;
    }
    // Unmask ep0 interrupts
    usbc->DAINTMSK = USBC_INEPMSK(1 << 0) | USBC_OUTEPMSK(1 << 0);
    usbc->DOEPMSK = USBC_XFERCOMPLMSK | USBC_SETUPMSK;
    usbc->DIEPMSK = USBC_XFERCOMPLMSK | USBC_TIMEOUTMSK;
    usbc->GINTMSK |= USBC_OEPINT | USBC_IEPINT
#ifdef USB_ISO
        | USBC_INCOMPISOOUT | USBC_INCOMPISOIN
#endif
        ;
#ifdef USB_SUSPEND
    usbc->GINTMSK &= ~USBC_WKUPINT;
#endif
    // Config ep0 size
    hal_usb_alloc_ep0_fifo();
    // Reset device address
    usbc->DCFG &= ~USBC_DEVADDR_MASK;

    hal_usb_init_ep0_transfer();
    hal_usb_init_epn_transfer();

    if (callbacks.state_change) {
        callbacks.state_change(HAL_USB_EVENT_RESET);
    }
}

static void hal_usb_irq_enum_done(void)
{
    uint8_t speed;
    uint8_t mps = 0;

    USB_FUNC_ENTRY_TRACE(2);

    speed = GET_BITFIELD(usbc->DSTS, USBC_ENUMSPD);
    if (speed == 0) {
        // High speed -- ERROR!
        mps = 0; // 64 bytes
    } else if (speed == 1 || speed == 3) {
        // Full speed
        mps = 0; // 64 bytes
    } else {
        // Low speed -- ERROR!
        mps = 3; // 8 bytes
    }
    // Only support 64-byte MPS !
    mps = 0;
    // Config max packet size
    usbc->DIEPCTL0 = USBC_EP0_MPS(mps) | USBC_USBACTEP | USBC_EPTXFNUM(0) | USBC_SNAK;
    usbc->DOEPCTL0 = USBC_EP0_MPS(mps) | USBC_USBACTEP | USBC_SNAK;

    hal_usb_recv_ep0();
}

#ifdef USB_ISO
static void hal_usb_irq_incomp_iso_out(void)
{
    int i;
    uint32_t ctrl;
    uint32_t sof_fn;
    uint32_t statusEp;

    sof_fn = GET_BITFIELD(usbc->DSTS, USBC_SOFFN);

    for (i = 0; i < EPNUM - 1; i++) {
        ctrl = usbc->DOEPnCONFIG[i].DOEPCTL;
        if ((ctrl & USBC_EPENA) && ((ctrl >> USBC_EPDPID_SHIFT) & 1) == (sof_fn & 1) &&
                GET_BITFIELD(ctrl, USBC_EPTYPE) == E_ISOCHRONOUS) {
            statusEp = usbc->DOEPnCONFIG[i].DOEPINT;
            hal_usb_disable_ep(EP_OUT, i + 1);
            break;
        }
    }

    if (i < EPNUM - 1) {
        USB_TRACE(17, "%s ep%d: INT=0x%08x, SOF=0x%04x", __FUNCTION__, i + 1, statusEp, sof_fn);
    } else {
        USB_TRACE(17, "%s: No valid ISO ep", __FUNCTION__);
    }
}

static void hal_usb_irq_incomp_iso_in(void)
{
    int i;
    uint32_t ctrl;
    uint32_t sof_fn;
    uint32_t statusEp;

    sof_fn = GET_BITFIELD(usbc->DSTS, USBC_SOFFN);

    for (i = 0; i < EPNUM - 1; i++) {
        ctrl = usbc->DIEPnCONFIG[i].DIEPCTL;
        if ((ctrl & USBC_EPENA) && ((ctrl >> USBC_EPDPID_SHIFT) & 1) == (sof_fn & 1) &&
                GET_BITFIELD(ctrl, USBC_EPTYPE) == E_ISOCHRONOUS) {
            statusEp = usbc->DIEPnCONFIG[i].DIEPINT;
            hal_usb_disable_ep(EP_IN, i + 1);
            break;
        }
    }

    if (i < EPNUM - 1) {
        USB_TRACE(17, "%s ep%d: INT=0x%08x, SOF=0x%04x", __FUNCTION__, i + 1, statusEp, sof_fn);
    } else {
        USB_TRACE(17, "%s: No valid ISO ep", __FUNCTION__);
    }
}
#endif

#ifdef USB_SUSPEND
static void hal_usb_pin_status_resume(enum PMU_USB_PIN_CHK_STATUS_T status)
{
    USB_TRACE(18, "%s: %d", __FUNCTION__, status);

    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, HAL_CMU_FREQ_52M);
    hal_cmu_clock_enable(HAL_CMU_MOD_H_USBC);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_USB);
    NVIC_EnableIRQ(USB_IRQn);
}

static void hal_usb_suspend(void)
{
    USB_FUNC_ENTRY_TRACE(18);

    device_suspended = 1;

    if (callbacks.state_change) {
        callbacks.state_change(HAL_USB_EVENT_SUSPEND);
    }

    usbc->GINTMSK |= USBC_WKUPINT;

    // Disable USB IRQ to avoid errors when RESET/RESUME is detected before stopping USB clock
    NVIC_DisableIRQ(USB_IRQn);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_USB);
    hal_cmu_clock_disable(HAL_CMU_MOD_H_USBC);
    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, HAL_CMU_FREQ_32K);

    pmu_usb_enable_pin_status_check(PMU_USB_PIN_CHK_HOST_RESUME, hal_usb_pin_status_resume);
}

static void hal_usb_resume(void)
{
    USB_FUNC_ENTRY_TRACE(18);

    device_suspended = 0;

    usbc->GINTMSK &= ~USBC_WKUPINT;

    if (callbacks.state_change) {
        callbacks.state_change(HAL_USB_EVENT_RESUME);
    }
}
#endif

static void hal_usb_irq_handler(void)
{
    uint32_t status, rawStatus;
    uint32_t statusEp, rawStatusEp;
    uint32_t data;
    uint8_t  i;

    // Store interrupt flag and reset it
    rawStatus = usbc->GINTSTS;
    usbc->GINTSTS = rawStatus;

    status = rawStatus & (usbc->GINTMSK & (USBC_USBRST | USBC_ENUMDONE
#ifdef USB_ISO
        | USBC_INCOMPISOOUT | USBC_INCOMPISOIN
#endif
        | USBC_IEPINT | USBC_OEPINT
        | USBC_ERLYSUSP | USBC_USBSUSP | USBC_WKUPINT));

    USB_TRACE(1, "%s: 0x%08x / 0x%08x", __FUNCTION__, status, rawStatus);

    if (status & USBC_USBRST) {
        // Usb reset
        hal_usb_irq_reset();

        // We got a reset, and reseted the soft state machine: Discard all other
        // interrupt causes.
        status &= USBC_ENUMDONE;
    }

    if (status & USBC_ENUMDONE) {
        // Enumeration done
        hal_usb_irq_enum_done();
    }

#ifdef USB_ISO
    if (status & USBC_INCOMPISOOUT) {
        // Incomplete ISO OUT
        hal_usb_irq_incomp_iso_out();
    }

    if (status & USBC_INCOMPISOIN) {
        // Incomplete ISO IN
        hal_usb_irq_incomp_iso_in();
    }
#endif

    if (status & USBC_IEPINT) {
        // Tx
        data = usbc->DAINT & usbc->DAINTMSK;
        for (i = 0; i < EPNUM; ++i) {
            if (data & USBC_INEPMSK(1 << i)) {
                if (i == 0) {
                    // EP0
                    rawStatusEp = usbc->DIEPINT0;
                    usbc->DIEPINT0 = rawStatusEp;
                    statusEp = rawStatusEp & usbc->DIEPMSK;

                    if ((statusEp & USBC_TIMEOUT) || (statusEp & USBC_INTKNTXFEMP)) {
                        usbc->DIEPMSK &= ~USBC_INTKNTXFEMPMSK;
                        hal_usb_stall_ep(EP_IN, i);
                    } else if (statusEp & USBC_XFERCOMPL) {
                        // Handle ep0 command
                        hal_usb_handle_ep0_packet(EP_IN, rawStatusEp);
                    }
                } else {
                    rawStatusEp = usbc->DIEPnCONFIG[i - 1].DIEPINT;
                    usbc->DIEPnCONFIG[i - 1].DIEPINT = rawStatusEp;
                    statusEp = rawStatusEp & usbc->DIEPMSK;

                    if ((statusEp & USBC_TIMEOUT) || (statusEp & USBC_XFERCOMPL)) {
                        hal_usb_send_epn_complete(i, rawStatusEp);
                    }
                }
            }
        }
    }

    if (status & USBC_OEPINT) {
        // Rx
        data = usbc->DAINT & usbc->DAINTMSK;
        for (i = 0; i < EPNUM; ++i) {
            if (data & USBC_OUTEPMSK(1 << i)) {
                if (i == 0) {
                    rawStatusEp = usbc->DOEPINT0;
                    usbc->DOEPINT0 = rawStatusEp;
                    statusEp = rawStatusEp & usbc->DOEPMSK;

                    if (statusEp & USBC_OUTTKNEPDIS) {
                        usbc->DOEPMSK &= ~USBC_OUTTKNEPDISMSK;
                        hal_usb_stall_ep(EP_OUT, i);
                    } else if (statusEp & (USBC_XFERCOMPL | USBC_SETUP)) {
                        // Handle ep0 command
                        hal_usb_handle_ep0_packet(EP_OUT, rawStatusEp);
                    }
                } else {
                    rawStatusEp = usbc->DOEPnCONFIG[i - 1].DOEPINT;
                    usbc->DOEPnCONFIG[i - 1].DOEPINT = rawStatusEp;
                    statusEp = rawStatusEp & usbc->DOEPMSK;

                    if (statusEp & USBC_XFERCOMPL) {
                        hal_usb_recv_epn_complete(i, rawStatusEp);
                    }
                }
            }
        }
    }

#ifdef USB_SUSPEND
    if (status & USBC_USBSUSP) {
        hal_usb_suspend();
    }

    if (status & USBC_WKUPINT) {
        hal_usb_resume();
    }
#endif
}
