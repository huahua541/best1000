#ifndef __HAL_USB_H__
#define __HAL_USB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"

#define MAX_PACKET_SIZE                         64
#define EP0_MAX_PACKET_SIZE                     64

#define MAX_ISO_PACKET_SIZE                     768 // 192K sample rate, 16 bits, 2 channels

// H/w register bit field width limitation
#define EPN_MAX_PKTCNT                          ((1 << 10) - 1)
#define EPN_MAX_XFERSIZE                        ((1 << 19) - 1)

#define EPNUM                                   6
#define CTRL_EPNUM                              2

enum HAL_USB_API_MODE {
    HAL_USB_API_NONBLOCKING,
    HAL_USB_API_BLOCKING,
};

enum EP_DIR {
    EP_OUT,
    EP_IN,
};

enum CTRL_STAGE {
    NONE_STAGE,
    WAIT_SETUP_STAGE,
    SETUP_STAGE,
    DATA_OUT_STAGE,
    DATA_IN_STAGE,
    STATUS_OUT_STAGE,
    STATUS_IN_STAGE,
};

enum ZLP_STATE {
    ZLP_AUTO,
    ZLP_DISABLED,
};

enum XFER_COMPL_STATE {
    XFER_COMPL_SUCCESS,
    XFER_COMPL_ERROR,
};

enum HAL_USB_STATE_EVENT {
    HAL_USB_EVENT_RESET, // RESET event should be processed as quickly as possible
    HAL_USB_EVENT_DISCONNECT,
    HAL_USB_EVENT_SUSPEND,
    HAL_USB_EVENT_RESUME,
};

struct SETUP_PACKET {
    struct {
        uint8_t recipient : 5;
        uint8_t type : 2;
        uint8_t direction : 1;
    } bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
};

struct EP0_TRANSFER {
    enum CTRL_STAGE stage;
    uint8_t *data;
    uint16_t length;
    uint16_t trx_len;
    struct SETUP_PACKET setup_pkt;
};

typedef const uint8_t *(*HAL_USB_GET_DESCRIPTOR_CALLBACK)(uint8_t);
typedef bool (*HAL_USB_SETUPRECV_CALLBACK)(struct EP0_TRANSFER *);
typedef bool (*HAL_USB_DATARECV_CALLBACK)(struct EP0_TRANSFER *);
typedef bool (*HAL_USB_SETCFG_CALLBACK)(uint8_t);
typedef bool (*HAL_USB_SETITF_CALLBACK)(uint16_t, uint16_t);
typedef void (*HAL_USB_REMOTE_WAKEUP_CALLBACK)(int enable);
typedef void (*HAL_USB_STATE_CHANGE_CALLBACK)(enum HAL_USB_STATE_EVENT);
typedef bool (*HAL_USB_RECV_COMPL_CALLBACK)(const uint8_t *, uint32_t, enum XFER_COMPL_STATE);
typedef bool (*HAL_USB_SEND_COMPL_CALLBACK)(const uint8_t *, uint32_t, enum XFER_COMPL_STATE);

struct HAL_USB_CALLBACKS {
    HAL_USB_GET_DESCRIPTOR_CALLBACK device_desc;
    HAL_USB_GET_DESCRIPTOR_CALLBACK cfg_desc;
    HAL_USB_GET_DESCRIPTOR_CALLBACK string_desc;

    HAL_USB_SETUPRECV_CALLBACK setuprecv;
    HAL_USB_DATARECV_CALLBACK datarecv;
    HAL_USB_SETCFG_CALLBACK setcfg;
    HAL_USB_SETITF_CALLBACK setitf;
    HAL_USB_REMOTE_WAKEUP_CALLBACK set_remote_wakeup;
    HAL_USB_STATE_CHANGE_CALLBACK state_change;

    HAL_USB_RECV_COMPL_CALLBACK epn_recv_compl[EPNUM - 1];
    HAL_USB_SEND_COMPL_CALLBACK epn_send_compl[EPNUM - 1];
};

int hal_usb_open(const struct HAL_USB_CALLBACKS *c, enum HAL_USB_API_MODE m);
int hal_usb_reopen(const struct HAL_USB_CALLBACKS *c, uint8_t dcfg, uint8_t alt, uint16_t itf);
void hal_usb_close(void);

int hal_usb_remote_wakeup(int signal);
void hal_usb_detect_disconn(void);

int hal_usb_configured(void);
int hal_usb_suspended(void);

void hal_usb_activate_epn(enum EP_DIR dir, uint8_t ep, uint8_t type, uint16_t mps);
void hal_usb_deactivate_epn(enum EP_DIR dir, uint8_t ep);

void hal_usb_disable_ep(enum EP_DIR dir, uint8_t ep);

void hal_usb_stall_ep(enum EP_DIR dir, uint8_t ep);
void hal_usb_unstall_ep(enum EP_DIR dir, uint8_t ep);
int hal_usb_get_ep_stall_state(enum EP_DIR dir, uint8_t ep);

void hal_usb_stop_ep(enum EP_DIR dir, uint8_t ep);

int hal_usb_recv_epn(uint8_t ep, uint8_t *buffer, uint32_t size);
int hal_usb_send_epn(uint8_t ep, const uint8_t *buffer, uint32_t size, enum ZLP_STATE zlp);

#ifdef __cplusplus
}
#endif

#endif

