#ifndef __APP_BT_MEDIA_MANAGER_H__
#define __APP_BT_MEDIA_MANAGER_H__

#include "resources.h"
#ifdef __cplusplus
extern "C" {
#endif

#define  BT_STREAM_SBC            0x1
#define  BT_STREAM_MEDIA       0x2
#define  BT_STREAM_VOICE       0x4

#define BT_STREAM_TYPE_MASK   (BT_STREAM_SBC | BT_STREAM_MEDIA | BT_STREAM_VOICE)


enum APP_BT_MEDIA_MANAGER_ID_T {
	APP_BT_STREAM_MANAGER_START = 0,
	APP_BT_STREAM_MANAGER_STOP,
	APP_BT_STREAM_MANAGER_SWITCHTO_SCO,
	APP_BT_STREAM_MANAGER_STOP_MEDIA,
	APP_BT_STREAM_MANAGER_NUM,
};


typedef struct {
    uint8_t id;
    uint8_t stream_type;
    uint8_t device_id;
    uint8_t aud_id;

}APP_AUDIO_MANAGER_MSG_STRUCT;

#define APP_AUDIO_MANAGER_SET_MESSAGE(appevt, id, stream_type) (appevt = (((uint32_t)id&0xffff)<<16)|(stream_type&0xffff))
#define APP_AUDIO_MANAGER_SET_MESSAGE0(appmsg, device_id,aud_id) (appmsg = (((uint32_t)device_id&0xffff)<<16)|(aud_id&0xffff))
#define APP_AUDIO_MANAGER_GET_ID(appevt, id) (id = (appevt>>16)&0xffff)
#define APP_AUDIO_MANAGER_GET_STREAM_TYPE(appevt, stream_type) (stream_type = appevt&0xffff)
#define APP_AUDIO_MANAGER_GET_DEVICE_ID(appmsg, device_id) (device_id = (appmsg>>16)&0xffff)
#define APP_AUDIO_MANAGER_GET_AUD_ID(appmsg, aud_id) (aud_id = appmsg&0xffff)

int app_audio_manager_sendrequest(uint8_t massage_id,uint8_t stream_type, uint8_t device_id, uint8_t aud_id);
void app_audio_manager_open(void);

void  bt_media_start(uint8_t stream_type,enum BT_DEVICE_ID_T device_id,AUD_ID_ENUM media_id);
void bt_media_stop(uint8_t stream_type,enum BT_DEVICE_ID_T device_id);
void bt_media_switch_to_voice(uint8_t stream_type,enum BT_DEVICE_ID_T device_id);
uint8_t bt_media_is_media_active_by_type(uint8_t media_type);
#ifdef __cplusplus
	}
#endif

#endif
