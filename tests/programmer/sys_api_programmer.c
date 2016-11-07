#include "stdint.h"
#include "string.h"
#include "usb_cdc.h"
#include "sys_api_programmer.h"
#include "hal_timer_raw.h"
#include "hal_uart.h"
#include "hal_cmu.h"
#include "hal_bootmode.h"
#include "hal_norflash.h"
#include "hal_iomux.h"
#include "flash_programmer.h"
#include "task_schedule.h"
#include "export_fn_rom.h"
#include "tool_msg.h"
#include "cmsis_nvic.h"
#include "pmu.h"

#define FLASH_BASE                              0x6C000000

enum UART_DMA_STATE {
    UART_DMA_IDLE,
    UART_DMA_START,
    UART_DMA_DONE,
    UART_DMA_ERROR,
};

static void setup_download_task(void);
static void setup_flash_task(void);

static const uint32_t default_recv_timeout_ms[XFER_TIMEOUT_QTY] = { 500, 1000, 2000, 10 * 60 * 1000, };
static const uint32_t default_send_timeout_ms = 500;

static uint32_t send_timeout;
static uint32_t recv_timeout;
static enum XFER_TIMEOUT cur_recv_timeout_type;

static const struct HAL_UART_CFG_T uart_cfg = {
    .parity = HAL_UART_PARITY_NONE,
    .stop = HAL_UART_STOP_BITS_1,
    .data = HAL_UART_DATA_BITS_8,
    .flow = HAL_UART_FLOW_CONTROL_NONE, //HAL_UART_FLOW_CONTROL_RTSCTS,
    .tx_level = HAL_UART_FIFO_LEVEL_1_2,
    .rx_level = HAL_UART_FIFO_LEVEL_1_2,
    .baud = 921600,
    .dma_rx = true,
    .dma_tx = false,
    .dma_rx_stop_on_err = false,
};
static const enum HAL_UART_ID_T dld_uart = HAL_UART_ID_0;
static const enum HAL_TRACE_TRANSPORT_T main_trace_transport = HAL_TRACE_TRANSPORT_UART0;
static const enum HAL_TRACE_TRANSPORT_T alt_trace_transport = HAL_TRACE_TRANSPORT_UART1;
static volatile enum UART_DMA_STATE uart_dma_rx_state = UART_DMA_IDLE;
static volatile uint32_t uart_dma_rx_size = 0;
static bool uart_opened = false;

static bool usb_opened = false;

static volatile bool cancel_xfer = false;
static enum DOWNLOAD_TRANSPORT dld_transport;

static uint32_t xfer_err_time = 0;
static uint32_t xfer_err_cnt = 0;

static void uart_rx_dma_handler(uint32_t xfer_size, int dma_error, union HAL_UART_IRQ_T status)
{
    // The DMA transfer has been cancelled
    if (uart_dma_rx_state != UART_DMA_START) {
        return;
    }

    uart_dma_rx_size = xfer_size;
    if (dma_error || status.FE || status.OE || status.PE || status.RT || status.BE) {
        TRACE("UART-RX Error: dma_error=%d, status=0x%08x", dma_error, status.reg);
        uart_dma_rx_state = UART_DMA_ERROR;
    } else {
        uart_dma_rx_state = UART_DMA_DONE;
    }
}

static void uart_break_handler(void)
{
    TRACE("****** Handle break ******");

    cancel_xfer = true;
    hal_uart_stop_dma_recv(dld_uart);
    hal_uart_stop_dma_send(dld_uart);
}

static void usb_serial_break_handler(void)
{
    TRACE("****** Handle break ******");

    cancel_xfer = true;
    hal_timer_stop();
    usb_serial_cancel_recv();
    usb_serial_cancel_send();
}

static void reopen_usb_serial(void)
{
    usb_serial_reopen(usb_serial_break_handler);
}

static void init_flash(void)
{
    int ret;

    static const struct HAL_NORFLASH_CONFIG_T cfg = {
#ifdef FPGA
        .source_clk = HAL_NORFLASH_SPEED_13M * 2,
        .speed = HAL_NORFLASH_SPEED_13M,
#else
        .source_clk = HAL_NORFLASH_SPEED_104M * 2,
        .speed = HAL_NORFLASH_SPEED_104M,
#endif
        .mode  = DRV_NORFLASH_OP_MODE_QUAD | DRV_NORFLASH_OP_MODE_CONTINUOUS_READ,
        .override_config = 0,
    };

    // Workaround for reboot: controller in SPI mode while FLASH in QUAD mode
    *(volatile uint32_t *)FLASH_BASE;

    hal_cmu_pll_enable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SYS);
    hal_cmu_flash_select_pll(HAL_CMU_PLL_AUD);
    hal_cmu_flash_set_freq(HAL_CMU_FREQ_208M);

    ret = hal_norflash_open(HAL_NORFLASH_ID_0, &cfg);

    ASSERT(ret == 0, "Failed to open norflash: %d", ret);
}

void set_download_transport(enum DOWNLOAD_TRANSPORT transport)
{
    union HAL_UART_IRQ_T mask;

    dld_transport = transport;

    if (transport == TRANSPORT_USB) {
        if (!usb_opened) {
#ifndef NO_UART_IOMUX
            hal_iomux_set_uart0_p3_0();
#endif
            reopen_usb_serial();
            usb_opened = true;
        }
    } else if (transport == TRANSPORT_UART) {
        hal_trace_switch(alt_trace_transport);

        if (!uart_opened) {
#ifndef NO_UART_IOMUX
            if (alt_trace_transport == HAL_TRACE_TRANSPORT_UART1) {
                hal_iomux_set_uart1_p3_2();
            }
#endif
            hal_uart_reopen(dld_uart, &uart_cfg);
            mask.reg = 0;
            mask.BE = 1;
            hal_uart_irq_set_dma_handler(dld_uart, uart_rx_dma_handler, NULL, uart_break_handler);
            hal_uart_irq_set_mask(dld_uart, mask);
            uart_opened = true;
        }
    }

    setup_download_task();
    setup_flash_task();
    init_flash();
}

static int uart_download_enabled(void)
{
    return !!(hal_sw_bootmode_get() & HAL_SW_BOOTMODE_DLD_TRANS_UART);
}

void init_download_transport(void)
{
    enum DOWNLOAD_TRANSPORT transport;

    transport = uart_download_enabled() ? TRANSPORT_UART : TRANSPORT_USB;
    set_download_transport(transport);
}

void init_transport(void)
{
    cancel_xfer = false;

    if (dld_transport == TRANSPORT_USB) {
        usb_serial_flush_recv_buffer();
        usb_serial_init_xfer();
    } else {
        hal_uart_flush(dld_uart);
    }

    cur_recv_timeout_type = XFER_TIMEOUT_QTY;
    set_recv_timeout(XFER_TIMEOUT_SHORT);
    set_send_timeout(XFER_TIMEOUT_SHORT);
}

void set_recv_timeout(enum XFER_TIMEOUT timeout)
{
    if (timeout < XFER_TIMEOUT_QTY) {
        if (timeout != cur_recv_timeout_type) {
            cur_recv_timeout_type = timeout;
            recv_timeout = MS_TO_TICKS(default_recv_timeout_ms[timeout]);
        }
    }
}

void set_send_timeout(enum XFER_TIMEOUT timeout)
{
    send_timeout = MS_TO_TICKS(default_send_timeout_ms);
}

int debug_read_enabled(void)
{
    return !!(hal_sw_bootmode_get() & HAL_SW_BOOTMODE_READ_ENABLED);
}

int debug_write_enabled(void)
{
    return !!(hal_sw_bootmode_get() & HAL_SW_BOOTMODE_WRITE_ENABLED);
}

static void usb_send_timeout(uint32_t elapsed)
{
    usb_serial_cancel_send();
}

static void usb_send_timer_start(void)
{
    hal_timer_setup(HAL_TIMER_TYPE_ONESHOT, usb_send_timeout);
    hal_timer_start(send_timeout);
}

static void usb_send_timer_stop(void)
{
    hal_timer_stop();
}

static int usb_send_data(const unsigned char *buf, size_t len)
{
    int ret;

    usb_send_timer_start();
    ret = usb_serial_send(buf, len);
    usb_send_timer_stop();
    return ret;
}

static int uart_send_data(const unsigned char *buf, size_t len)
{
    uint32_t start;
    uint32_t sent = 0;

    start = hal_sys_timer_get();
    while (sent < len) {
        while (!cancel_xfer && !hal_uart_writable(dld_uart) &&
                hal_sys_timer_get() - start < send_timeout &&
                TASK_SCHEDULE);
        if (cancel_xfer) {
            break;
        }
        if (hal_uart_writable(dld_uart)) {
            hal_uart_putc(dld_uart, buf[sent++]);
        } else {
            break;
        }
    }

    if (sent != len) {
        return 1;
    }

    return 0;
}

int send_data(const unsigned char *buf, size_t len)
{
    if (cancel_xfer) {
        return -1;
    }

    if (dld_transport == TRANSPORT_USB) {
        return usb_send_data(buf, len);
    } else {
        return uart_send_data(buf, len);
    }
}

static void usb_recv_timeout(uint32_t elapsed)
{
    usb_serial_cancel_recv();
}

static void usb_recv_timer_start(void)
{
    hal_timer_setup(HAL_TIMER_TYPE_ONESHOT, usb_recv_timeout);
    hal_timer_start(recv_timeout);
}

static void usb_recv_timer_stop(void)
{
    hal_timer_stop();
}

static int usb_recv_data(unsigned char *buf, size_t len, size_t *rlen)
{
    int ret;

    usb_recv_timer_start();
    ret = usb_serial_recv(buf, len);
    usb_recv_timer_stop();
    if (ret == 0) {
        *rlen = len;
    }
    return ret;
}

static int usb_recv_data_dma(unsigned char *buf, size_t len, size_t expect, size_t *rlen)
{
    int ret;

    usb_recv_timer_start();
    ret = usb_serial_direct_recv(buf, len, expect, rlen, 0, NULL);
    usb_recv_timer_stop();
    return ret;
}

static int uart_recv_data_dma(unsigned char *buf, size_t len, size_t expect, size_t *rlen)
{
    int ret;
    union HAL_UART_IRQ_T mask;
    uint32_t start;
    struct HAL_DMA_DESC_T dma_desc[17];
    uint32_t desc_cnt = ARRAY_SIZE(dma_desc);

    if (uart_dma_rx_state != UART_DMA_IDLE) {
        ret = -3;
        goto _no_state_exit;
    }

    uart_dma_rx_state = UART_DMA_START;
    uart_dma_rx_size = 0;

    ret = hal_uart_dma_recv(dld_uart, buf, expect, &dma_desc[0], &desc_cnt);
    if (ret) {
        goto _exit;
    }

    mask.reg = 0;
    mask.BE = 1;
    mask.FE = 1;
    mask.OE = 1;
    mask.PE = 1;
    //mask.RT = 1;
    hal_uart_irq_set_mask(dld_uart, mask);

    start = hal_sys_timer_get();
    while (uart_dma_rx_state == UART_DMA_START && !cancel_xfer &&
            hal_sys_timer_get() - start < recv_timeout &&
            TASK_SCHEDULE);

    if (cancel_xfer) {
        ret = -14;
        goto _exit;
    }
    if (uart_dma_rx_state == UART_DMA_START) {
        hal_uart_stop_dma_recv(dld_uart);
        ret = -15;
        goto _exit;
    }
    if (uart_dma_rx_state != UART_DMA_DONE) {
        ret = -2;
        goto _exit;
    }

    ret = 0;
    *rlen = uart_dma_rx_size;

_exit:
    mask.reg = 0;
    mask.BE = 1;
    hal_uart_irq_set_mask(dld_uart, mask);

    uart_dma_rx_state = UART_DMA_IDLE;

_no_state_exit:
    return ret;
}

static int uart_recv_data(unsigned char *buf, size_t len, size_t *rlen)
{
    return uart_recv_data_dma(buf, len, len, rlen);
}

int recv_data_dma(unsigned char *buf, size_t len, size_t expect, size_t *rlen)
{
    if (cancel_xfer) {
        return -1;
    }

    if (dld_transport == TRANSPORT_USB) {
        return usb_recv_data_dma(buf, len, expect, rlen);
    } else {
        return uart_recv_data_dma(buf, len, expect, rlen);
    }
}

int recv_data(unsigned char *buf, size_t len, size_t *rlen)
{
    if (cancel_xfer) {
        return -1;
    }

    if (dld_transport == TRANSPORT_USB) {
        return usb_recv_data(buf, len, rlen);
    } else {
        return uart_recv_data(buf, len, rlen);
    }
}

static int usb_handle_error(void)
{
    int ret;

    TRACE("****** Send break ******");

    // Send break signal, to tell the peer to reset the connection
    ret = usb_serial_send_break();
    if (ret) {
        TRACE("Sending break failed: %d", ret);
    }
    return ret;
}

static int uart_handle_error(void)
{
    TRACE("****** Send break ******");

    // Send break signal, to tell the peer to reset the connection
    hal_uart_break_set(dld_uart);
    hal_sys_timer_delay(MS_TO_TICKS(100));
    hal_uart_break_clear(dld_uart);

    return 0;
}

int handle_error(void)
{
    int ret = 0;
    uint32_t err_time;

    hal_sys_timer_delay(MS_TO_TICKS(200));

    if (!cancel_xfer) {
        if (dld_transport == TRANSPORT_USB) {
            ret = usb_handle_error();
        } else {
            ret = uart_handle_error();
        }
    }

    xfer_err_cnt++;
    err_time = hal_sys_timer_get();
    if (err_time - xfer_err_time > MS_TO_TICKS(5000)) {
        xfer_err_cnt = 0;
    }
    xfer_err_time = err_time;
    if (xfer_err_cnt < 2) {
        hal_sys_timer_delay(MS_TO_TICKS(500));
    } else if (xfer_err_cnt < 5) {
        hal_sys_timer_delay(MS_TO_TICKS(1000));
    } else {
        hal_sys_timer_delay(MS_TO_TICKS(2000));
    }

    return ret;
}

static int usb_cancel_input(void)
{
    return usb_serial_flush_recv_buffer();
}

static int uart_cancel_input(void)
{
    hal_uart_flush(dld_uart);
    return 0;
}

int cancel_input(void)
{
    if (dld_transport == TRANSPORT_USB) {
        return usb_cancel_input();
    } else {
        return uart_cancel_input();
    }
}

void system_reboot(void)
{
    hal_cmu_sys_reboot();
}

void system_shutdown(void)
{
	pmu_shutdown();
}

void system_flash_boot(void)
{
    __export_fn_rom.boot_from_flash_reent();
}

void system_set_bootmode(unsigned int bootmode)
{
    bootmode &= ~(HAL_SW_BOOTMODE_READ_ENABLED | HAL_SW_BOOTMODE_WRITE_ENABLED);
    hal_sw_bootmode_set(bootmode);
}

void system_clear_bootmode(unsigned int bootmode)
{
    bootmode &= ~(HAL_SW_BOOTMODE_READ_ENABLED | HAL_SW_BOOTMODE_WRITE_ENABLED);
    hal_sw_bootmode_clear(bootmode);
}

unsigned int system_get_bootmode(void)
{
    return hal_sw_bootmode_get();
}

int get_sector_info(unsigned int addr, unsigned int *sector_addr, unsigned int *sector_len)
{
    hal_norflash_get_boundary(HAL_NORFLASH_ID_0, addr, sector_addr, NULL);
    hal_norflash_get_size(HAL_NORFLASH_ID_0, NULL, sector_len, NULL, NULL);
    return 0;
}

int erase_sector(unsigned int sector_addr, unsigned int sector_len)
{
    hal_norflash_erase(HAL_NORFLASH_ID_0, sector_addr, sector_len);
    return 0;
}

int erase_chip(void)
{
    hal_norflash_erase_chip(HAL_NORFLASH_ID_0);
    return 0;
}

int burn_data(unsigned int addr, const unsigned char *data, unsigned int len)
{
    hal_norflash_write(HAL_NORFLASH_ID_0, addr, data, len);
    return 0;
}

int wait_data_buf_finished(void)
{
    while (free_data_buf()) {
        task_schedule();
    }

    return 0;
}

int wait_all_data_buf_finished(void)
{
    while (data_buf_in_use()) {
        while (free_data_buf() == 0);
        task_schedule();
    }

    return 0;
}

// ========================================================================
// Programmer Task

extern uint32_t __StackTop[];
extern uint32_t __StackLimit[];
extern uint32_t __bss_start__[];
extern uint32_t __bss_end__[];

#define EXEC_STRUCT_LOC                 __attribute__((section(".exec_struct")))

void programmer_main(void)
{
    uint32_t *dst;
    int ret;

    for (dst = __bss_start__; dst < __bss_end__; dst++) {
        *dst = 0;
    }

#ifdef PROGRAMMER_LOAD_SIMU

    hal_cmu_simu_pass();

#else // !PROGRAMMER_LOAD_SIMU

#ifdef DEBUG
    for (dst = (uint32_t *)(__get_MSP() - 4); dst >= __StackLimit; dst--) {
        *dst = 0xCCCCCCCC;
    }
#endif

    NVIC_InitVectors();

    hal_sys_timer_open();
    hal_gpdma_open();
    hal_trace_open(uart_download_enabled() ? alt_trace_transport : main_trace_transport);
    ret = hal_analogif_open();
    ASSERT(ret == 0, "Failed to open analogif");
    ret = pmu_open();
    ASSERT(ret == 0, "Failed to open pmu");

    TRACE("------ Enter %s ------", __FUNCTION__);

    programmer_loop();

    TRACE("------ Exit %s ------", __FUNCTION__);

#endif // !PROGRAMMER_LOAD_SIMU

    while (1);
}

void programmer_start(void)
{
    __set_MSP((uint32_t)__StackTop);
    programmer_main();
}

const struct exec_struct_t EXEC_STRUCT_LOC exec_struct = {
    .entry = (uint32_t)programmer_start,
    .param = 0,
    .sp = 0,
};


// ========================================================================
// Flash Task

#define STACK_ALIGN                     ALIGNED(8)
#define FLASH_TASK_STACK_SIZE           (2048)

static unsigned char STACK_ALIGN flash_task_stack[FLASH_TASK_STACK_SIZE + 16];

static void setup_download_task(void)
{
    int ret;

    // Fake entry for task 0
    ret = task_setup(0, 0, false, NULL);

    ASSERT(ret == 0, "Failed to setup download task: %d", ret);
}

static void flash_task(void)
{
    unsigned int index;

    while (1) {
        while (get_burn_data_buf(&index)) {
            task_schedule();
        }

        handle_data_buf(index);
    }
}

static void setup_flash_task(void)
{
    int ret;

#ifdef DEBUG
    memset(&flash_task_stack[0], 0xcc, sizeof(flash_task_stack));
#endif

    ret = task_setup(1, (uint32_t)flash_task_stack + FLASH_TASK_STACK_SIZE, true, flash_task);

    ASSERT(ret == 0, "Failed to setup flash task: %d", ret);
}

