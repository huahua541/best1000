#ifndef __HAL_KEY_H__
#define __HAL_KEY_H__

#include "hal_gpio.h"
#include "hal_gpadc.h"

#ifdef __cplusplus
extern "C" {
#endif

enum HAL_KEY_CODE_T {
	HAL_KEY_CODE_PWR = 0,
	HAL_KEY_CODE_FN1,
	HAL_KEY_CODE_FN2,
	HAL_KEY_CODE_FN3,
	HAL_KEY_CODE_FN4,
	HAL_KEY_CODE_FN5,
	HAL_KEY_CODE_FN6,
	HAL_KEY_CODE_FN7,
	HAL_KEY_CODE_FN8,
	HAL_KEY_CODE_FN9,
	HAL_KEY_CODE_FN10,
	HAL_KEY_CODE_FN11,
	HAL_KEY_CODE_FN12,
	HAL_KEY_CODE_FN13,
	HAL_KEY_CODE_FN14,
	HAL_KEY_CODE_FN15,
	HAL_KEY_CODE_NONE,

	HAL_KEY_CODE_NUM,
};

enum HAL_KEY_EVENT_T {
	HAL_KEY_EVENT_NONE = 0,
	HAL_KEY_EVENT_DEBOUNCE,
	HAL_KEY_EVENT_DITHERING,
	HAL_KEY_EVENT_DOWN,
	HAL_KEY_EVENT_UP,
	HAL_KEY_EVENT_LONGPRESS,
	HAL_KEY_EVENT_LONGPRESS3S,
	HAL_KEY_EVENT_LONGLONGPRESS,
	HAL_KEY_EVENT_CLICK,
	HAL_KEY_EVENT_DOUBLECLICK,
	HAL_KEY_EVENT_REPEAT,
	HAL_KEY_EVENT_INITDOWN,
	HAL_KEY_EVENT_INITUP,
	HAL_KEY_EVENT_INITLONGPRESS,
	HAL_KEY_EVENT_INITLONGLONGPRESS,
	HAL_KEY_EVENT_INITFINISHED,

	HAL_KEY_EVENT_NUM,
};

struct HAL_KEY_ADCKEY_T {
	HAL_GPADC_MV_T volt;
	enum HAL_KEY_EVENT_T event;
	uint32_t time;
};

struct HAL_KEY_GPIOKEY_T {
	enum HAL_GPIO_PIN_T pin;
	enum HAL_KEY_EVENT_T event;
	uint32_t time;
};

struct HAL_KEY_PWRKEY_T {
	enum HAL_KEY_EVENT_T event;
	uint32_t time;
};

struct HAL_KEY_STATUS_T {
	struct HAL_KEY_PWRKEY_T pwr_key;
	struct HAL_KEY_ADCKEY_T adc_key;
	struct HAL_KEY_GPIOKEY_T gpio_key;
	enum HAL_KEY_CODE_T key_code;
	enum HAL_KEY_EVENT_T event;
	uint32_t click_time;
};

struct HAL_KEY_GPIOKEY_CFG_T {
	enum HAL_KEY_CODE_T key_code;
	struct HAL_IOMUX_PIN_FUNCTION_MAP key_config;
};

int hal_key_open(int checkPwrKey, int (* cb)(uint8_t, uint8_t));
enum HAL_KEY_EVENT_T hal_bt_key_read(enum HAL_KEY_CODE_T code);

int hal_key_close(void);

#ifdef __cplusplus
	}
#endif

#endif//__FMDEC_H__
