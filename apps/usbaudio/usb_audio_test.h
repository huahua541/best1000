#ifndef __USB_AUDIO_TEST_H__
#define __USB_AUDIO_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "plat_types.h"
#include "hal_key.h"

void usb_audio_tester(bool on);

void usb_audio_tester_init(uint8_t *play_buf, uint32_t play_size,
                           uint8_t *cap_buf, uint32_t cap_size,
                           uint8_t *recv_buf, uint32_t recv_size,
                           uint8_t *send_buf, uint32_t send_size);

void usb_audio_tester_term(void);
void app_uaudio_open(uint8_t *ply_buf,uint8_t *cap_buf,uint8_t *recv_buf,uint8_t *send_buf);


#define USB_AUDIO_PLAYBACK_BUFF_SIZE    (FRAME_SIZE_PLAYBACK * 4)
#define USB_AUDIO_CAPTURE_BUFF_SIZE     (FRAME_SIZE_CAPTURE * 4)

#define USB_AUDIO_RECV_BUFF_SIZE        (FRAME_SIZE_RECV * 8)
#define USB_AUDIO_SEND_BUFF_SIZE        (FRAME_SIZE_SEND * 8)


void usb_audio_tester_key(enum HAL_KEY_CODE_T code, enum HAL_KEY_EVENT_T event);

enum USB_NOTIFIER{
   USB_NOTIFI_MOD_KEY = 0,
   USB_NOTIFI_MOD_DRV,
   USB_NOTIFI_MOD_MAX
};
//void notify_app_uaudio_param(enum USB_NOTIFIER id, uint32_t msg);
int app_uaudio_mailbox_put(uint32_t id,uint32_t arg);

#ifdef __cplusplus
}
#endif

#endif
