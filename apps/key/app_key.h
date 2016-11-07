#ifndef __APP_KEY_H__
#define __APP_KEY_H__

#include "hal_key.h"

#define APP_KEY_SET_MESSAGE(appevt, code, evt) (appevt = (((uint32_t)code&0xffff)<<16)|(evt&0xffff))
#define APP_KEY_GET_CODE(appevt, code) (code = (appevt>>16)&0xffff)
#define APP_KEY_GET_EVENT(appevt, evt) (evt = appevt&0xffff)

enum APP_KEY_CODE_T {
    APP_KEY_CODE_PWR =  HAL_KEY_CODE_PWR,
    APP_KEY_CODE_FN1 =  HAL_KEY_CODE_FN1,
    APP_KEY_CODE_FN2 =  HAL_KEY_CODE_FN2,
    APP_KEY_CODE_FN3 =  HAL_KEY_CODE_FN3, 
    APP_KEY_CODE_FN4 =  HAL_KEY_CODE_FN4,
    APP_KEY_CODE_FN5 =  HAL_KEY_CODE_FN5,
    APP_KEY_CODE_FN6 =  HAL_KEY_CODE_FN6,
    APP_KEY_CODE_FN7 =  HAL_KEY_CODE_FN7,
    APP_KEY_CODE_FN8 =  HAL_KEY_CODE_FN8,
    APP_KEY_CODE_FN9 =  HAL_KEY_CODE_FN9,
    APP_KEY_CODE_FN10 = HAL_KEY_CODE_FN10,
    APP_KEY_CODE_FN11 = HAL_KEY_CODE_FN11,
    APP_KEY_CODE_FN12 = HAL_KEY_CODE_FN12,
    APP_KEY_CODE_FN13 = HAL_KEY_CODE_FN13,
    APP_KEY_CODE_FN14 = HAL_KEY_CODE_FN14,
    APP_KEY_CODE_FN15 = HAL_KEY_CODE_FN15,
    APP_KEY_CODE_NONE = HAL_KEY_CODE_NONE,
                 
    APP_KEY_CODE_NUM =  HAL_KEY_CODE_NUM 
};

enum APP_KEY_EVENT_T {
    APP_KEY_EVENT_NONE              =   HAL_KEY_EVENT_NONE,
    APP_KEY_EVENT_DEBOUNCE          =   HAL_KEY_EVENT_DEBOUNCE,     
    APP_KEY_EVENT_DITHERING         =   HAL_KEY_EVENT_DITHERING,      
    APP_KEY_EVENT_DOWN              =   HAL_KEY_EVENT_DOWN,         
    APP_KEY_EVENT_UP                =   HAL_KEY_EVENT_UP,           
    APP_KEY_EVENT_LONGPRESS         =   HAL_KEY_EVENT_LONGPRESS,
    APP_KEY_EVENT_LONGPRESS3S       =   HAL_KEY_EVENT_LONGPRESS3S,
    APP_KEY_EVENT_LONGLONGPRESS     =   HAL_KEY_EVENT_LONGLONGPRESS,    
    APP_KEY_EVENT_CLICK             =   HAL_KEY_EVENT_CLICK,        
    APP_KEY_EVENT_DOUBLECLICK       =   HAL_KEY_EVENT_DOUBLECLICK,
    APP_KEY_EVENT_REPEAT            =   HAL_KEY_EVENT_REPEAT,
    APP_KEY_EVENT_INITDOWN          =   HAL_KEY_EVENT_INITDOWN,     
    APP_KEY_EVENT_INITUP            =   HAL_KEY_EVENT_INITUP,       
    APP_KEY_EVENT_INITLONGPRESS     =   HAL_KEY_EVENT_INITLONGPRESS,
    APP_KEY_EVENT_INITLONGLONGPRESS =   HAL_KEY_EVENT_INITLONGLONGPRESS,
    APP_KEY_EVENT_INITFINISHED      =   HAL_KEY_EVENT_INITFINISHED,
                                                                      
    APP_KEY_EVENT_NUM               =   HAL_KEY_EVENT_NUM,     
};


typedef struct {
    uint16_t code;
    uint16_t event;
}APP_KEY_STATUS;

typedef void (*APP_KEY_HANDLE_CB_T)(APP_KEY_STATUS*, void *param);

typedef struct {
    APP_KEY_STATUS key_status;
    const char* string;
    APP_KEY_HANDLE_CB_T function;
    void *param;
} APP_KEY_HANDLE;

int app_key_open(int checkPwrKey);

int app_key_close(void);

int app_key_handle_registration(const APP_KEY_HANDLE *key_handle);

void app_key_handle_clear(void);

#endif//__FMDEC_H__
