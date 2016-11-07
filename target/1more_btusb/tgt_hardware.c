#include "cmsis_os.h"
#include "tgt_hardware.h"
#include "hal_iomux.h"
#include "hal_gpio.h"
#include "hal_key.h"

const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_pinmux_pwl[CFG_HW_PLW_NUM] = {
    {HAL_IOMUX_PIN_P2_5, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE}, //set blue led
    {HAL_IOMUX_PIN_P2_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE}, //set red led
};

//adckey define
const uint16_t CFG_HW_ADCKEY_MAP_TABLE[CFG_HW_ADCKEY_NUMBER] = {
    HAL_KEY_CODE_FN9,HAL_KEY_CODE_FN8,HAL_KEY_CODE_FN7,
    HAL_KEY_CODE_FN6,HAL_KEY_CODE_FN5,HAL_KEY_CODE_FN4,
    HAL_KEY_CODE_FN3,HAL_KEY_CODE_FN2,HAL_KEY_CODE_FN1
};

//gpiokey define
#define CFG_HW_GPIOKEY_DOWN_LEVEL          (0)
#define CFG_HW_GPIOKEY_UP_LEVEL            (1)
const struct HAL_KEY_GPIOKEY_CFG_T cfg_hw_gpio_key_cfg[CFG_HW_GPIOKEY_NUM] = {
    {HAL_KEY_CODE_FN1,{HAL_IOMUX_PIN_P3_6, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE}},
    {HAL_KEY_CODE_FN2,{HAL_IOMUX_PIN_P3_7, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE}},
};

//bt config
const char *BT_LOCAL_NAME = "1more_test\0";
uint8_t ble_addr[6] = {0xBE,0x99,0x34,0x45,0x56,0x67};
uint8_t bt_addr[6] = {0x30,0x05,0x34,0x45,0x56,0x67};

//audio config
//freq bands range {[0k:2.5K], [2.5k:5K], [5k:7.5K], [7.5K:10K], [10K:12.5K], [12.5K:15K], [15K:17.5K], [17.5K:20K]}
//gain range -12~+12
const int8_t cfg_hw_aud_eq_band_settings[CFG_HW_AUD_EQ_NUM_BANDS] = {0, 0, 0, 0, 0, 0, 0, 0};

const struct  CODEC_DAC_VOL_T codec_dac_vol[CODEC_DAC_VOL_LEVEL_NUM]={
    {0x10,0x03,0x0F},
    {0x00,0x03,0x00},
    {0x00,0x03,0x01},
    {0x00,0x03,0x01},
    {0x01,0x03,0x01},
    {0x01,0x03,0x03},
    {0x01,0x03,0x05},
    {0x01,0x03,0x08},
    {0x01,0x03,0x0C},
    {0x01,0x03,0x0E},
    {0x06,0x03,13},
    {0x06,0x03,15},
    {0x06,0x03,17},
    {0x06,0x03,19},
    {0x08,0x03,20},
    {0x09,0x03,22},
    {0x0A,0x03,24}   //0dBm
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_detecter_cfg = {
    HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_chg_en = {
    HAL_IOMUX_PIN_P2_3, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE //charger en
};


