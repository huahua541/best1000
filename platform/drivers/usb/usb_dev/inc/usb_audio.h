#ifndef __USB_AUDIO_H__
#define __USB_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

enum USB_AUDIO_STATE_EVENT_T {
    USB_AUDIO_STATE_RESET, // RESET event should be processed as quickly as possible
    USB_AUDIO_STATE_DISCONNECT,
    USB_AUDIO_STATE_SLEEP,
    USB_AUDIO_STATE_WAKEUP,
    USB_AUDIO_STATE_CONFIG,
};

enum USB_AUDIO_HID_EVENT_T {
    USB_AUDIO_HID_VOL_UP        = (1 << 0),
    USB_AUDIO_HID_VOL_DOWN      = (1 << 1),
    USB_AUDIO_HID_PLAY          = (1 << 2),
    USB_AUDIO_HID_SCAN_NEXT     = (1 << 3),
    USB_AUDIO_HID_SCAN_PREV     = (1 << 4),
    USB_AUDIO_HID_STOP          = (1 << 5),
    USB_AUDIO_HID_FAST_FWD      = (1 << 6),
    USB_AUDIO_HID_REWIND        = (1 << 7),
    USB_AUDIO_HID_REDIAL        = (1 << 8),
    USB_AUDIO_HID_HOOK_SWITCH   = (1 << 9),
    USB_AUDIO_HID_MIC_MUTE      = (1 << 10),
};

typedef void (*USB_AUDIO_START_CALLBACK)(uint32_t start);
typedef void (*USB_AUDIO_MUTE_CALLBACK)(uint32_t mute);
typedef void (*USB_AUDIO_SET_VOLUME)(uint32_t percent);
typedef uint32_t (*USB_AUDIO_GET_VOLUME)(void);
typedef void (*USB_AUDIO_XFER_CALLBACK)(const uint8_t *data, uint32_t size, int error);
typedef void (*USB_AUDIO_STATE_CALLBACK)(enum USB_AUDIO_STATE_EVENT_T event);

struct USB_AUDIO_CFG_T {
    USB_AUDIO_START_CALLBACK recv_start_callback;
    USB_AUDIO_START_CALLBACK send_start_callback;
    USB_AUDIO_MUTE_CALLBACK mute_callback;
    USB_AUDIO_SET_VOLUME set_volume;
    USB_AUDIO_GET_VOLUME get_volume;
    USB_AUDIO_STATE_CALLBACK state_callback;
    USB_AUDIO_XFER_CALLBACK recv_callback;
    USB_AUDIO_XFER_CALLBACK send_callback;
};

int usb_audio_open(const struct USB_AUDIO_CFG_T *cfg);

void usb_audio_close(void);

int usb_audio_recv_ready(void);

int usb_audio_send_ready(void);

int usb_audio_start_recv(uint8_t *buf, uint32_t pos, uint32_t size);

void usb_audio_stop_recv(void);

int usb_audio_start_send(const uint8_t *buf, uint32_t pos, uint32_t size);

void usb_audio_stop_send(void);

void usb_audio_hid_set_event(enum USB_AUDIO_HID_EVENT_T event, int state);

#ifdef __cplusplus
}
#endif

#endif
