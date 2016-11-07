#ifndef __HAL_TRACE_H__
#define __HAL_TRACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NORETURN                    __attribute__((noreturn))

#ifdef DEBUG
#define FUNC_ENTRY_TRACE()          hal_trace_printf(__FUNCTION__)
#define TRACE(str, ...)             hal_trace_printf(str, ##__VA_ARGS__)
#define DUMP8(str, buf, cnt)        hal_trace_dump(str, sizeof(uint8_t), cnt, buf)
#define DUMP16(str, buf, cnt)       hal_trace_dump(str, sizeof(uint16_t), cnt, buf)
#define DUMP32(str, buf, cnt)       hal_trace_dump(str, sizeof(uint32_t), cnt, buf)
#else
#define FUNC_ENTRY_TRACE()
#define TRACE(...)
#define DUMP8(str, buf, cnt)
#define DUMP16(str, buf, cnt)
#define DUMP32(str, buf, cnt)
#endif
#define ASSERT(cond, str, ...)      { if (!(cond)) { hal_trace_crash_dump(str, ##__VA_ARGS__); } }

enum HAL_TRACE_TRANSPORT_T {
    HAL_TRACE_TRANSPORT_USB,
    HAL_TRACE_TRANSPORT_UART0,
    HAL_TRACE_TRANSPORT_UART1,

    HAL_TRACE_TRANSPORT_QTY
};

int hal_trace_switch(enum HAL_TRACE_TRANSPORT_T transport);

int hal_trace_open(enum HAL_TRACE_TRANSPORT_T transport);

int hal_trace_output(const unsigned char *buf, unsigned int buf_len);

int hal_trace_printf(const char *fmt, ...);
int hal_trace_printf_without_crlf(const char *fmt, ...);

int hal_trace_dump (const char *fmt, unsigned int size,  unsigned int count, const void *buffer);
int hal_trace_busy(void);

void hal_trace_idle_output(void);

void NORETURN hal_trace_crash_dump(const char *fmt, ...);

int hal_trace_tportopen(void);

int hal_trace_tportset(int port);

int hal_trace_tportclr(int port);

#ifdef __cplusplus
}
#endif

#endif
