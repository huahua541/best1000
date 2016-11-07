#include "hal_trace.h"
#include "hal_uart.h"
#include "hal_iomux.h"
#include "hal_gpio.h"
#include "hal_dma.h"
#include "hal_cmu.h"
#include "pmu.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "cmsis.h"

#define TRACE_IDLE_OUTPUT   0
#define TRACE_BUF_SIZE      (4 * 1024)

struct HAL_TRACE_BUF_T {
    unsigned char buf[TRACE_BUF_SIZE];
    unsigned short wptr;
    unsigned short rptr;
#if (TRACE_IDLE_OUTPUT == 0)
    unsigned short sends[2];
#endif
    unsigned short discards;
    bool sending;
    bool in_trace;
};

static const struct HAL_UART_CFG_T uart_cfg = {
    .parity = HAL_UART_PARITY_NONE,
    .stop = HAL_UART_STOP_BITS_1,
    .data = HAL_UART_DATA_BITS_8,
    .flow = HAL_UART_FLOW_CONTROL_NONE,//HAL_UART_FLOW_CONTROL_RTSCTS,
    .tx_level = HAL_UART_FIFO_LEVEL_1_2,
    .rx_level = HAL_UART_FIFO_LEVEL_1_4,
    .baud = 921600,
    .dma_rx = false,
#if (TRACE_IDLE_OUTPUT == 0)
    .dma_tx = true,
#else
    .dma_tx = false,
#endif
    .dma_rx_stop_on_err = false,
};

#if (TRACE_IDLE_OUTPUT == 0)
static struct HAL_DMA_CH_CFG_T dma_cfg;

static struct HAL_DMA_DESC_T dma_desc[2];
#endif

static enum HAL_TRACE_TRANSPORT_T trace_transport = HAL_TRACE_TRANSPORT_QTY;
static enum HAL_UART_ID_T trace_uart;

static struct HAL_TRACE_BUF_T trace;

#ifdef DEBUG
static char crash_buf[100];
#endif

static bool trace_init = false;

#if (TRACE_IDLE_OUTPUT == 0)

static void hal_trace_uart_send(void)
{
    uint32_t sends;
    uint32_t size;
    uint32_t int_state;

    int_state = int_lock();

    // There is a race condition if we do not check s/w flag, but only check the h/w status.
    // [e.g., hal_gpdma_chan_busy(dma_cfg.ch)]
    // When the DMA is done, but DMA IRQ handler is still pending due to interrupt lock
    // or higher priority IRQ, it will have a chance to send the same content twice.
    if (trace.wptr != trace.rptr && !trace.sending) {
        trace.sending = true;

        if (trace.wptr > trace.rptr) {
            sends = trace.wptr - trace.rptr;
        } else {
            sends = TRACE_BUF_SIZE - (trace.rptr - trace.wptr);
        }

        size = TRACE_BUF_SIZE - trace.rptr;
        dma_cfg.src = (uint32_t)&trace.buf[trace.rptr];
        if (size >= sends) {
            size = sends;
            dma_cfg.src_tsize = size;
            hal_gpdma_init_desc(&dma_desc[0], &dma_cfg, NULL, 1);
            trace.sends[1] = dma_cfg.src_tsize;
        } else {
            dma_cfg.src_tsize = size;
            hal_gpdma_init_desc(&dma_desc[0], &dma_cfg, &dma_desc[1], 0);
            trace.sends[0] = dma_cfg.src_tsize;

            dma_cfg.src = (uint32_t)&trace.buf[0];
            dma_cfg.src_tsize = sends - size;
            hal_gpdma_init_desc(&dma_desc[1], &dma_cfg, NULL, 1);
            trace.sends[1] = dma_cfg.src_tsize;
        }

        hal_gpdma_sg_start(&dma_desc[0], &dma_cfg);
    }

    int_unlock(int_state);
}

static void hal_trace_uart_xfer_done(uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    if (error) {
        if (lli) {
            if (trace.sends[0] > remain_tsize) {
                trace.sends[0] -= remain_tsize;
            } else {
                trace.sends[0] = 0;
            }
            trace.sends[1] = 0;
        } else {
            if (trace.sends[1] > remain_tsize) {
                trace.sends[1] -= remain_tsize;
            } else {
                trace.sends[1] = 0;
            }
        }
    }

    trace.rptr += trace.sends[0] + trace.sends[1];
    if (trace.rptr >= TRACE_BUF_SIZE) {
        trace.rptr -= TRACE_BUF_SIZE;
    }
    trace.sends[0] = 0;
    trace.sends[1] = 0;
    trace.sending = false;

    if (trace.wptr != trace.rptr) {
        hal_trace_uart_send();
    }
}

static void hal_trace_send(void)
{
    if (trace_transport == HAL_TRACE_TRANSPORT_QTY) {
        return;
    }

    hal_trace_uart_send();
}

#else // TRACE_IDLE_OUTPUT

static void hal_trace_uart_idle_send(void)
{
    uint32_t sends;
    uint32_t size;
    int i;

    if (trace.wptr == trace.rptr) {
        return;
    }

    if (trace.wptr > trace.rptr) {
        sends = trace.wptr - trace.rptr;
    } else {
        sends = TRACE_BUF_SIZE - (trace.rptr - trace.wptr);
    }

    size = TRACE_BUF_SIZE - trace.rptr;
    if (size >= sends) {
        size = sends;
        for (i = trace.rptr; i < size; i++) {
            hal_uart_blocked_putc(trace_uart, trace.buf[i]);
        }
    } else {
        for (i = trace.rptr; i < size; i++) {
            hal_uart_blocked_putc(trace_uart, trace.buf[i]);
        }
        for (i = 0; i < sends - size; i++) {
            hal_uart_blocked_putc(trace_uart, trace.buf[i]);
        }
    }

    trace.rptr += sends;
    if (trace.rptr >= TRACE_BUF_SIZE) {
        trace.rptr -= TRACE_BUF_SIZE;
    }
}

void hal_trace_idle_send(void)
{
    if (trace_transport == HAL_TRACE_TRANSPORT_QTY) {
        return;
    }

    hal_trace_uart_idle_send();
}

#endif // TRACE_IDLE_OUTPUT

int hal_trace_switch(enum HAL_TRACE_TRANSPORT_T transport)
{
    uint32_t lock;
    int ret;

#ifdef FORCE_TRACE_UART1
    transport = HAL_TRACE_TRANSPORT_UART1;
#endif

    if (trace_transport >= HAL_TRACE_TRANSPORT_QTY) {
        return 1;
    }
    if (trace_transport == transport) {
        return 0;
    }

    ret = 0;

    lock = int_lock();

    if (trace_transport == HAL_TRACE_TRANSPORT_UART0 || trace_transport == HAL_TRACE_TRANSPORT_UART1) {
#if (TRACE_IDLE_OUTPUT == 0)
        if (dma_cfg.ch != HAL_DMA_CHAN_NONE) {
            hal_gpdma_cancel(dma_cfg.ch);
        }
#endif
        hal_uart_close(trace_uart);
    }

    if (transport == HAL_TRACE_TRANSPORT_UART0 || transport == HAL_TRACE_TRANSPORT_UART1) {
        if (transport == HAL_TRACE_TRANSPORT_UART0) {
            trace_uart = HAL_UART_ID_0;
        } else if (transport == HAL_TRACE_TRANSPORT_UART1) {
            trace_uart = HAL_UART_ID_1;
        }
#if (TRACE_IDLE_OUTPUT == 0)
        dma_cfg.dst_periph = (trace_uart == HAL_UART_ID_0) ? HAL_GPDMA_UART0_TX : HAL_GPDMA_UART1_TX;
        trace.sends[0] = 0;
        trace.sends[1] = 0;
#endif
        ret = hal_uart_open(trace_uart, &uart_cfg);
        if (ret) {
#if (TRACE_IDLE_OUTPUT == 0)
            hal_gpdma_free_chan(dma_cfg.ch);
            dma_cfg.ch = HAL_DMA_CHAN_NONE;
#endif
            trace_transport = HAL_TRACE_TRANSPORT_QTY;
            goto _exit;
        }
    }

    trace.sending = false;

    trace_transport = transport;

_exit:
    int_unlock(lock);

    return ret;
}

int hal_trace_open(enum HAL_TRACE_TRANSPORT_T transport)
{
    int ret;

#ifdef FORCE_TRACE_UART1
    transport = HAL_TRACE_TRANSPORT_UART1;
#endif

    if (transport == HAL_TRACE_TRANSPORT_USB ||
            transport >= HAL_TRACE_TRANSPORT_QTY) {
        return 1;
    }

    if (trace_transport != HAL_TRACE_TRANSPORT_QTY) {
        return hal_trace_switch(transport);
    }

    if (transport == HAL_TRACE_TRANSPORT_UART0) {
        trace_uart = HAL_UART_ID_0;
    } else if (transport == HAL_TRACE_TRANSPORT_UART1) {
        trace_uart = HAL_UART_ID_1;
    }

    trace.wptr = 0;
    trace.rptr = 0;
    trace.discards = 0;
    trace.sending = false;
    trace.in_trace = false;

#if (TRACE_IDLE_OUTPUT == 0)
    trace.sends[0] = 0;
    trace.sends[1] = 0;

    if (transport == HAL_TRACE_TRANSPORT_UART0 ||
            transport == HAL_TRACE_TRANSPORT_UART1) {
        memset(&dma_cfg, 0, sizeof(dma_cfg));
        dma_cfg.ch = hal_gpdma_get_chan(HAL_DMA_HIGH_PRIO);
        dma_cfg.dst = 0; // useless
        dma_cfg.dst_bsize = HAL_DMA_BSIZE_8;
        dma_cfg.dst_periph = (trace_uart == HAL_UART_ID_0) ? HAL_GPDMA_UART0_TX : HAL_GPDMA_UART1_TX;
        dma_cfg.dst_width = HAL_DMA_WIDTH_BYTE;
        dma_cfg.handler = hal_trace_uart_xfer_done;
        dma_cfg.src_bsize = HAL_DMA_BSIZE_32;
        dma_cfg.src_periph = 0; // useless
        dma_cfg.src_width = HAL_DMA_WIDTH_BYTE;
        dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;
        dma_cfg.try_burst = 0;

        ASSERT(dma_cfg.ch != HAL_DMA_CHAN_NONE, "Failed to get DMA channel");
        ASSERT(trace_uart == HAL_UART_ID_0 || trace_uart == HAL_UART_ID_1,
            "BAD trace uart ID: %d", trace_uart);
    }
#endif

    ret = hal_uart_open(trace_uart, &uart_cfg);
    if (ret) {
#if (TRACE_IDLE_OUTPUT == 0)
        hal_gpdma_free_chan(dma_cfg.ch);
        dma_cfg.ch = HAL_DMA_CHAN_NONE;
#endif
        return ret;
    }

    trace_transport = transport;

    trace_init = true;

    return 0;
}

int hal_trace_output(const unsigned char *buf, unsigned int buf_len)
{
    int ret;
    uint32_t lock;
    uint32_t avail;
    uint16_t size;

    if (!trace_init)
        return -1;
    ret = 0;

    lock = int_lock();

    // Avoid troubles when NMI occurs during trace
    if (!trace.in_trace) {
        trace.in_trace = true;

        if (trace.wptr >= trace.rptr) {
            avail = TRACE_BUF_SIZE - (trace.wptr - trace.rptr) - 1;
        } else {
            avail = TRACE_BUF_SIZE - (trace.rptr - trace.wptr) - 1;
        }

        if (avail < buf_len) {
            trace.discards++;
            ret = 1;
        } else {
            trace.discards = 0;

            size = TRACE_BUF_SIZE - trace.wptr;
            if (size >= buf_len) {
                size = buf_len;
            }
            memcpy(&trace.buf[trace.wptr], &buf[0], size);
            if (size < buf_len) {
                memcpy(&trace.buf[0], &buf[size], buf_len - size);
            }
            trace.wptr += buf_len;
            if (trace.wptr >= TRACE_BUF_SIZE) {
                trace.wptr -= TRACE_BUF_SIZE;
            }
#if (TRACE_IDLE_OUTPUT == 0)
            hal_trace_send();
#endif
        }

        trace.in_trace = false;
    }

    int_unlock(lock);

    return ret ? 0 : buf_len;
}

int hal_trace_printf(const char *fmt, ...)
{
    char buf[100];
    int len;
    va_list ap;

    if (trace.discards > 0) {
        len = snprintf(buf, sizeof(buf), "\r\nLOST %d\r\n", trace.discards);
    } else {
        len = 0;
    }

    va_start(ap, fmt);
    len += vsnprintf(&buf[len], sizeof(buf) - len, fmt, ap);
    if (len + 2 < sizeof(buf)) {
        buf[len++] = '\r';
    }
    if (len + 1 < sizeof(buf)) {
        buf[len++] = '\n';
    }
    //buf[len] = 0;
    va_end(ap);

    return hal_trace_output((unsigned char *)buf, len);
}


int hal_trace_printf_without_crlf(const char *fmt, ...)
{
    char buf[100];
    int len;
    va_list ap;

    if (trace.discards > 0) {
        len = snprintf(buf, sizeof(buf), "\r\nLOST %d\r\n", trace.discards);
    } else {
        len = 0;
    }

    va_start(ap, fmt);
    len += vsnprintf(&buf[len], sizeof(buf) - len, fmt, ap);
    //buf[len] = 0;
    va_end(ap);

    return hal_trace_output((unsigned char *)buf, len);
}


int hal_trace_dump (const char *fmt, unsigned int size,  unsigned int count, const void *buffer)
{
	char buf[255];
	int len=0,n=0,i=0;

	if (trace.discards > 0) {
		len = snprintf(buf, sizeof(buf), "\r\nLOST %d\r\n", trace.discards);
	} else {
		len = 0;
	}

	switch( size )
	{
		case sizeof(uint32_t):
			while(i<count && len<sizeof(buf))
			{
				len += snprintf(&buf[len], sizeof(buf) - len, fmt, *(uint32_t *)((uint32_t *)buffer+i));
				i++;
			}
			break;
		case sizeof(uint16_t):
				while(i<count && len<sizeof(buf))
				{
					len += snprintf(&buf[len], sizeof(buf) - len, fmt, *(uint16_t *)((uint16_t *)buffer+i));
					i++;
				}
				break;
		case sizeof(uint8_t):
		default:
			while(i<count && len<sizeof(buf))
			{
				len += snprintf(&buf[len], sizeof(buf) - len, fmt, *(uint8_t *)((uint8_t *)buffer+i));
				i++;
			}
			break;
	}


    if (len + 2 < sizeof(buf)) {
        buf[len++] = '\r';
    }
    if (len + 1 < sizeof(buf)) {
        buf[len++] = '\n';
    }

	n = hal_trace_output((unsigned char *)buf, len);

	return n;
}

int hal_trace_busy(void)
{
    union HAL_UART_FLAG_T flag;

    flag = hal_uart_get_flag(trace_uart);

    return flag.BUSY;
}

#ifdef DEBUG
static void hal_trace_flush_buffer(void)
{
#if (TRACE_IDLE_OUTPUT == 0)
    while (trace.wptr != trace.rptr) {
        while (hal_gpdma_chan_busy(dma_cfg.ch));
        hal_gpdma_irq_run_chan(dma_cfg.ch);
    }
#else
    while (trace.wptr != trace.rptr) {
        hal_trace_idle_send();
    }
#endif
}
#endif

void hal_trace_crash_dump(const char *fmt, ...)
{
#ifdef DEBUG
    int len;
    va_list ap;

    int_lock();

    hal_trace_flush_buffer();

    len = snprintf(crash_buf, sizeof(crash_buf), "\r\nLOST %d\r\n### ASSERT ###\r\n", trace.discards);

    va_start(ap, fmt);
    len += vsnprintf(&crash_buf[len], sizeof(crash_buf) - len, fmt, ap);
    if (len + 2 < sizeof(crash_buf)) {
        crash_buf[len++] = '\r';
    }
    if (len + 1 < sizeof(crash_buf)) {
        crash_buf[len++] = '\n';
    }
    //buf[len] = 0;
    va_end(ap);

    hal_trace_output((unsigned char *)crash_buf, len);

    hal_trace_flush_buffer();
#endif

    // Tag failure for simulation envvironment
    hal_cmu_simu_fail();
    pmu_crash_config();
    const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_iic_bt[] = {
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_I2C_SCL, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_I2C_SDA, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_iic_bt, sizeof(pinmux_iic_bt)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));

    while (1);
}

void os_error_str (const char *fmt, ...) __attribute__((alias("hal_trace_crash_dump")));

const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_tport[] = {
    {HAL_IOMUX_PIN_P1_7, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    {HAL_IOMUX_PIN_P1_6, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    {HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    {HAL_IOMUX_PIN_P1_4, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
};

int hal_trace_tportopen(void)
{
    int i;

    for (i=0;i<sizeof(pinmux_tport)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP);i++){
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&pinmux_tport[i], 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)pinmux_tport[i].pin, HAL_GPIO_DIR_OUT, 0);
    }
    return 0;
}

int hal_trace_tportset(int port)
{
    hal_gpio_pin_set((enum HAL_GPIO_PIN_T)pinmux_tport[port].pin);
    return 0;
}

int hal_trace_tportclr(int port)
{
    hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)pinmux_tport[port].pin);
    return 0;
}

