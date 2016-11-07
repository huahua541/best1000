#ifndef __CFG_HARDWARE__
#define __CFG_HARDWARE__
#include "hal_iomux.h"
#include "hal_gpio.h"
#include "hal_key.h"

#ifdef __cplusplus
extern "C" {
#endif

//pwl
#define CFG_HW_PLW_NUM (2)
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_pinmux_pwl[CFG_HW_PLW_NUM];

//adckey define
#define CFG_HW_ADCKEY_NUMBER 9
#define CFG_HW_ADCKEY_BASE 0
#define CFG_HW_ADCKEY_ADC_MAXVOLT 1000
#define CFG_HW_ADCKEY_ADC_MINVOLT 0
#define CFG_HW_ADCKEY_ADC_KEYVOLT_BASE 130
extern const uint16_t CFG_HW_ADCKEY_MAP_TABLE[CFG_HW_ADCKEY_NUMBER];

#define BTA_AV_CO_SBC_MAX_BITPOOL  53

//gpiokey define
#define CFG_HW_GPIOKEY_NUM (2)
#define CFG_HW_GPIOKEY_DOWN_LEVEL          (0)
#define CFG_HW_GPIOKEY_UP_LEVEL            (1)
extern const struct HAL_KEY_GPIOKEY_CFG_T cfg_hw_gpio_key_cfg[CFG_HW_GPIOKEY_NUM];

// audio codec
#define CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV (ANALOG_AUD_ADC_A)
#define CFG_HW_AUD_INPUT_PATH_HP_MIC_DEV (ANALOG_AUD_ADC_A | ANALOG_AUD_ADC_B)
#define CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV (ANALOG_AUD_LDAC |ANALOG_AUD_RDAC)

//bt config
extern const char *BT_LOCAL_NAME;
extern uint8_t ble_addr[6];
extern uint8_t bt_addr[6];

//audio config
struct  CODEC_DAC_VOL_T{
    uint32_t tx_pa_gain:5;  //reg:0x33, bit:6
    uint32_t sdm_gain:2;    //reg:0x4000A048, bit:10
    uint32_t sdac_volume:5; //reg:0x4000A048, bit:2
};

//mic
//step 6db
#define ANALOG_GA1A_GAIN (4)
//step 0.75db
#define ANALOG_GA2A_GAIN (7)
//step 1db
#define CODEC_SADC_VOL (7)

enum TGT_VOLUME_LEVEL_T {
    TGT_VOLUME_LEVE_WARNINGTONE = 0,

    TGT_VOLUME_LEVE_0 = 1,
    TGT_VOLUME_LEVE_1 = 2,
    TGT_VOLUME_LEVE_2 = 3,
    TGT_VOLUME_LEVE_3 = 4,
    TGT_VOLUME_LEVE_4 = 5,
    TGT_VOLUME_LEVE_5 = 6,
    TGT_VOLUME_LEVE_6 = 7,
    TGT_VOLUME_LEVE_7 = 8,
    TGT_VOLUME_LEVE_8 = 9,
    TGT_VOLUME_LEVE_9 = 10,
    TGT_VOLUME_LEVE_10 = 11,
    TGT_VOLUME_LEVE_11 = 12,
    TGT_VOLUME_LEVE_12 = 13,
    TGT_VOLUME_LEVE_13 = 14,
    TGT_VOLUME_LEVE_14 = 15,
    TGT_VOLUME_LEVE_15 = 16,

    TGT_VOLUME_LEVE_QTY = 17
};

typedef struct {
    float fc; /* Hz */
    float q; /* q  bw=fc/q*/
    float gain;   /* +/- dB */
}TGT_AUD_EQ_IIR_BAND_SETTINGS;
 
#define CODEC_DAC_VOL_LEVEL_NUM (TGT_VOLUME_LEVE_QTY)
extern const struct CODEC_DAC_VOL_T codec_dac_vol[CODEC_DAC_VOL_LEVEL_NUM];

//range -12~+12
#define CFG_HW_AUD_EQ_NUM_BANDS (8)
extern const int8_t cfg_aud_eq_sbc_band_settings[CFG_HW_AUD_EQ_NUM_BANDS];
#define CFG_AUD_EQ_IIR_NUM_BANDS (4)
extern const TGT_AUD_EQ_IIR_BAND_SETTINGS cfg_aud_eq_iir_band_settings[CFG_AUD_EQ_IIR_NUM_BANDS];

//battery info
#define APP_BATTERY_MIN_MV (3300)
#define APP_BATTERY_PD_MV   (3100)

#define APP_BATTERY_MAX_MV (4200)

extern const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_detecter_cfg;
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_chg_en;

#ifdef __cplusplus
}
#endif

#endif

