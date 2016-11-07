#ifndef __PMU_H__
#define __PMU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "hal_analogif.h"

#define pmu_read(reg,val)  hal_analogif_reg_read(reg,val)
#define pmu_write(reg,val) hal_analogif_reg_write(reg,val)

union SECURITY_VALUE_T {
    struct {
        unsigned short security_en      :1;
        unsigned short key_id           :3;
        unsigned short key_chksum       :2;
        unsigned short vendor_id        :6;
        unsigned short vendor_chksum    :3;
        unsigned short chksum           :1;
    };
    unsigned short reg;
};

enum PMU_EFUSE_PAGE_T {
    PMU_EFUSE_PAGE_SECURITY     = 0,
    PMU_EFUSE_PAGE_BOOT         = 1,
    PMU_EFUSE_PAGE_RESERVED_1   = 2,
    PMU_EFUSE_PAGE_BATTER_LV    = 3,

    PMU_EFUSE_PAGE_BATTER_HV    = 4,
    PMU_EFUSE_PAGE_RESERVED_2   = 5,
    PMU_EFUSE_PAGE_RESERVED_3   = 6,
    PMU_EFUSE_PAGE_RESERVED_4   = 7,

    PMU_EFUSE_PAGE_RESERVED_5   = 8,
    PMU_EFUSE_PAGE_RESERVED_6   = 9,
    PMU_EFUSE_PAGE_RESERVED_7   = 10,
    PMU_EFUSE_PAGE_RESERVED_8   = 11,

    PMU_EFUSE_PAGE_RESERVED_9   = 12,
    PMU_EFUSE_PAGE_RESERVED_10  = 13,
    PMU_EFUSE_PAGE_RESERVED_11  = 14,
    PMU_EFUSE_PAGE_RESERVED_12  = 15,
};


enum PMU_MODUAL_T
{
    PMU_ANA,
    PMU_DIG,
    PMU_IO,
    PMU_MEM,
    PMU_GP,
    PMU_USB,
    PMU_CRYSTAL,
    PMU_FM,
    PMU_PA,
    PMU_CODEC,
};

enum PMU_CHARGER_STATUS_T {
    PMU_CHARGER_PLUGIN,
    PMU_CHARGER_PLUGOUT,
    PMU_CHARGER_UNKNOWN,
};

typedef struct
{
    unsigned char module;
    unsigned short manual_bit;
    unsigned short ldo_en;
    unsigned short lp_mode;
    unsigned short dsleep_mode;
    unsigned short dsleep_v;
    unsigned short dsleep_v_shift;
    unsigned short normal_v;
    unsigned short  normal_v_shift;
}PMU_MODULE_CFG_T;

enum PMU_USB_PIN_CHK_STATUS_T {
    PMU_USB_PIN_CHK_NONE,
    // Chip acts as host
    PMU_USB_PIN_CHK_DEV_CONN,
    // Chip acts as host
    PMU_USB_PIN_CHK_DEV_DISCONN,
    // Chip acts as device
    PMU_USB_PIN_CHK_HOST_RESUME,

    PMU_USB_PIN_CHK_STATUS_QTY,
};

typedef void (*PMU_USB_PIN_CHK_CALLBACK)(enum PMU_USB_PIN_CHK_STATUS_T status);


#define  PMU_MANUAL_MODE                1
#define  PMU_AUTO_MODE                  0
#define  PMU_LDO_ON                     1
#define  PMU_LDO_OFF                    0
#define  PMU_LP_MODE_ON                 1
#define  PMU_LP_MODE_OFF                0
#define  PMU_DSLEEP_MODE_ON             1
#define  PMU_DSLEEP_MODE_OFF            0

#define  BEST1000_LDO_MODE              0
#define  BEST1000_ANA_DCDC_MODE         1
#define  BEST1000_DIG_DCDC_MODE         2


#define  PMU_VMEM_3_0V   0x16
#define  PMU_VMEM_2_3V   0x9
#define  PMU_VMEM_2_2V   0x8
#define  PMU_VMEM_2_1V   0x7
#define  PMU_VMEM_2_0V   0x6
#define  PMU_VMEM_1_9V   0x5
#define  PMU_VMEM_1_8V   0x4

#define  PMU_VDIG_1_4V    0xF
#define  PMU_VDIG_1_3V    0xE
#define  PMU_VDIG_1_2V    0xC
#define  PMU_VDIG_1_1V    0xA
#define  PMU_VDIG_1_0V    0x8
#define  PMU_VDIG_0_9V    0x6
#define  PMU_VDIG_0_8V    0x4
#define  PMU_VDIG_0_7V    0x2
#define  PMU_VDIG_0_6V    0x0

#define  PMU_DCDC_ANA_2_3V   0xF
#define  PMU_DCDC_ANA_2_2V   0xE
#define  PMU_DCDC_ANA_2_1V   0xD
#define  PMU_DCDC_ANA_2_0V   0xC
#define  PMU_DCDC_ANA_1_9V   0xB
#define  PMU_DCDC_ANA_1_8V   0xA
#define  PMU_DCDC_ANA_1_7V   0x9
#define  PMU_DCDC_ANA_1_6V   0x8

#define  PMU_DCDC_ANA_0_8V   0x3
#define  PMU_DCDC_ANA_0_7V   0x2
#define  PMU_DCDC_ANA_0_6V   0x1

#define  PMU_DCDC_ANA_SLEEP_1_6V   0xF

#define PMU_DCDC_DIG_1_3V    0x5
#define PMU_DCDC_DIG_1_2V    0x4
#define PMU_DCDC_DIG_1_1V    0x3
#define PMU_DCDC_DIG_1_0V    0x2
#define PMU_DCDC_DIG_0_9V    0x1
#define PMU_DCDC_DIG_0_8V    0x0

#define PMU_IO_2_8V         0x10
#define PMU_IO_2_7V         0xF
#define PMU_IO_2_6V         0xE
#define PMU_IO_2_5V         0xD
#define PMU_IO_2_4V         0xC
#define PMU_IO_2_3V         0xB
#define PMU_IO_2_2V         0xa
#define PMU_IO_2_1V         0x9
#define PMU_IO_2_0V         0x8
#define PMU_IO_1_9V         0x7
#define PMU_IO_1_8V         0x6
#define PMU_IO_1_7V         0x5

#define PMU_CODEC_3_5V      0xD
#define PMU_CODEC_3_4V      0xC
#define PMU_CODEC_3_3V      0xB
#define PMU_CODEC_3_2V      0xA
#define PMU_CODEC_3_1V      0x9
#define PMU_CODEC_3_0V      0x8
#define PMU_CODEC_2_9V      0x7
#define PMU_CODEC_2_8V      0x6
#define PMU_CODEC_2_7V      0x5
#define PMU_CODEC_2_6V      0x4
#define PMU_CODEC_2_5V      0x3
#define PMU_CODEC_2_4V      0x2
#define PMU_CODEC_2_3V      0x1
#define PMU_CODEC_2_2V      0x0


#define  PMU_HIGH_DCDC_ANA_2_4V   0x9
#define  PMU_HIGH_DCDC_ANA_2_2V   0x8
#define  PMU_HIGH_DCDC_ANA_2_0V   0x7
#define  PMU_HIGH_DCDC_ANA_1_9V   0x6
#define  PMU_HIGH_DCDC_ANA_1_7V   0x5


#define  PMU_HIGH_DCDC_ANA_SLEEP_2_3V   0xF
#define  PMU_HIGH_DCDC_ANA_SLEEP_2_2V   0xE
#define  PMU_HIGH_DCDC_ANA_SLEEP_2_1V   0xD
#define  PMU_HIGH_DCDC_ANA_SLEEP_2_0V   0xC
#define  PMU_HIGH_DCDC_ANA_SLEEP_1_9V   0xB
#define  PMU_HIGH_DCDC_ANA_SLEEP_1_8V   0xA
#define  PMU_HIGH_DCDC_ANA_SLEEP_1_7V   0x9


int pmu_open(void);

void pmu_dcdc_ana_set_volt(unsigned short normal_v,unsigned short dsleep_v);

void pmu_dcdc_ana_get_volt(unsigned short *normal_v,unsigned short *dsleep_v);

void pmu_module_config(enum PMU_MODUAL_T module,unsigned short is_manual,unsigned short ldo_on,unsigned short lp_mode,unsigned short dpmode);

void pmu_mode_change(unsigned char  mode);

void pmu_charger_plugin_config(void);

void pmu_charger_plugout_config(void);

void pmu_module_set_volt(unsigned char module,unsigned short  sleep_v,unsigned short normal_v);

int pmu_charger_detected(void);

int pmu_get_security_value(union SECURITY_VALUE_T *val);

void pmu_shutdown(void);

int pmu_get_efuse(enum PMU_EFUSE_PAGE_T page, unsigned short *efuse);

void pmu_anc_config(int enable);

void pmu_usb_config(int enable);

void pmu_crash_config(void);

void pmu_sleep_en(unsigned char sleep_en);

void pmu_flash_write_config(void);

void pmu_flash_read_config(void);

void pmu_charger_init(void);

enum PMU_CHARGER_STATUS_T pmu_charger_get_status(void);

enum PMU_CHARGER_STATUS_T pmu_charger_detect(void);

int pmu_usb_enable_pin_status_check(enum PMU_USB_PIN_CHK_STATUS_T status, PMU_USB_PIN_CHK_CALLBACK callback);

void pmu_usb_disable_pin_status_check(void);

void pmu_usb_get_pin_status(int *dp, int *dn);

#ifdef __cplusplus
}
#endif

#endif

