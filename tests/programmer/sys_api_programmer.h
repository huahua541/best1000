#ifndef __SYS_API_H__
#define __SYS_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include "stdio.h"

#define TRACE(str, ...)                 do { printf("%s/" str "\n", __FUNCTION__, __VA_ARGS__); } while (0)
#define ASSERT(cond, str, ...)          \
    do { if (!(cond)) { printf("[ASSERT]%s/" str, __FUNCTION__, __VA_ARGS__); while (1); } } while (0)
#define TRACE_TIME(str, ...)            TRACE(str, __VA_ARGS__)

int write_flash_data(const unsigned char *data, unsigned int len);
#else
#include "hal_trace.h"
#include "hal_timer.h"

#define TRACE_TIME(str, ...)            TRACE("[%05u] " str, TICKS_TO_MS(hal_sys_timer_get()), ##__VA_ARGS__)
#endif

enum DOWNLOAD_TRANSPORT {
    TRANSPORT_USB,
    TRANSPORT_UART,
};

enum XFER_TIMEOUT {
    XFER_TIMEOUT_SHORT,
    XFER_TIMEOUT_MEDIUM,
    XFER_TIMEOUT_LONG,
    XFER_TIMEOUT_IDLE,

    XFER_TIMEOUT_QTY
};

void set_download_transport(enum DOWNLOAD_TRANSPORT transport);
void init_download_transport(void);

void init_transport(void);

void set_recv_timeout(enum XFER_TIMEOUT timeout);
void set_send_timeout(enum XFER_TIMEOUT timeout);

int send_data(const unsigned char *buf, size_t len);
int recv_data(unsigned char *buf, size_t len, size_t *rlen);
int recv_data_dma(unsigned char *buf, size_t len, size_t expect, size_t *rlen);
int handle_error(void);
int cancel_input(void);

int debug_read_enabled(void);
int debug_write_enabled(void);

void system_reboot(void);
void system_shutdown(void);
void system_flash_boot(void);
void system_set_bootmode(unsigned int bootmode);
void system_clear_bootmode(unsigned int bootmode);
unsigned int system_get_bootmode(void);
int get_sector_info(unsigned int addr, unsigned int *sector_addr, unsigned int *sector_len);
int erase_sector(unsigned int sector_addr, unsigned int sector_len);
int erase_chip(void);
int burn_data(unsigned int addr, const unsigned char *data, unsigned int len);

int wait_data_buf_finished(void);
int wait_all_data_buf_finished(void);

#ifdef __cplusplus
}
#endif

#endif

