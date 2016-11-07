#ifndef __HAL_GPADC_H__
#define __HAL_GPADC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_GPADC_BAD_VALUE  0xFFFF

enum HAL_GPADC_CHAN_T {
	HAL_GPADC_CHAN_0 = 0,
	HAL_GPADC_CHAN_BATTERY = 1,
	HAL_GPADC_CHAN_2 = 2,
	HAL_GPADC_CHAN_3 = 3,
	HAL_GPADC_CHAN_4 = 4,
	HAL_GPADC_CHAN_5 = 5,
	HAL_GPADC_CHAN_6 = 6,
	HAL_GPADC_CHAN_ADCKEY = 7,
	HAL_GPADC_CHAN_QTY,
};

enum HAL_GPADC_ATP_T {
	HAL_GPADC_ATP_NULL = 0,
	HAL_GPADC_ATP_122US = 122,
	HAL_GPADC_ATP_1MS = 1000,
	HAL_GPADC_ATP_10MS = 10000,
    HAL_GPADC_ATP_20MS = 20000,
	HAL_GPADC_ATP_100MS = 100000,
	HAL_GPADC_ATP_250MS = 250000,
	HAL_GPADC_ATP_500MS = 500000,
	HAL_GPADC_ATP_1S = 1000000,
	HAL_GPADC_ATP_2S = 2000000,
    HAL_GPADC_ATP_ONESHOT = -1,
};

typedef uint16_t HAL_GPADC_MV_T;


typedef void (*HAL_GPADC_EVENT_CB_T)(void *);

int hal_gpadc_open(enum HAL_GPADC_CHAN_T channel, enum HAL_GPADC_ATP_T atp, HAL_GPADC_EVENT_CB_T cb);

int hal_gpadc_close(enum HAL_GPADC_CHAN_T channel);

void hal_gpadc_sleep(void);

void hal_gpadc_wakeup(void);

#ifdef __cplusplus
	}
#endif

#endif//__FMDEC_H__
