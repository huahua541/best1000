#include "cmsis.h"
#include "cmsis_nvic.h"
#include "pmu.h"
#include "hal_timer.h"
#include "hal_sleep.h"
#include "hal_trace.h"
#include "hal_chipid.h"

#ifdef CODEC_HIGH_QUALITY
#define PMU_CODEC_DEFAULT_VOLT          PMU_CODEC_3_5V
#else
#define PMU_CODEC_DEFAULT_VOLT          PMU_CODEC_2_7V
#endif

#define PMU_CHARGER_STATUS_IN           (1 << 0)

#define PMU_EFUSE_TRIGGER               (1 << 15)
#define PMU_EFUSE_CLK_EN                (1 << 12)
#define PMU_EFUSE_WRITE2                (1 << 11)
#define PMU_EFUSE_MODE                  (1 << 10)
#define PMU_EFUSE_REF_RES_BIT(n)        (((n) & 3) << 8)
#define PMU_EFUSE_REF_RES_BIT_MASK      (3 << 8)
#define PMU_EFUSE_REF_RES_BIT_SHIFT     (8)
#define PMU_EFUSE_PROG_TIME(n)          (((n) & 0xF) << 4)
#define PMU_EFUSE_PROG_TIME_MASK        (0xF << 4)
#define PMU_EFUSE_PROG_TIME_SHIFT       (4)
#define PMU_EFUSE_WORD_SEL(n)           (((n) & 0xF) << 0)
#define PMU_EFUSE_WORD_SEL_MASK         (0xF << 0)
#define PMU_EFUSE_WORD_SEL_SHIFT        (0)

///40   PMU_REG_POWER_OFF
#define PMU_METAL_ID_SHIFT              4
#define PMU_METAL_ID_MASK               (0xF << PMU_METAL_ID_SHIFT)
#define PMU_METAL_ID(n)                 (((n) & 0xF) << PMU_METAL_ID_SHIFT)
#define PMU_SOFT_POWER_OFF              (1 << 1)

///45   PMU_REG_USB_PIN_CFG
#define PMU_USB_PU_RX_SE                (1 << 13)
#define PMU_USB_POL_RX_DP               (1 << 12)
#define PMU_USB_POL_RX_DP_SHIFT         (12)
#define PMU_USB_POL_RX_DM               (1 << 11)
#define PMU_USB_POL_RX_DM_SHIFT         (11)
#define PMU_USB_DEBOUNCE_EN             (1 << 10)
#define PMU_USB_NOLS_MODE               (1 << 9)
#define PMU_USB_INSERT_DET_EN           (1 << 8)

///48   ana setting
#define PMU_ANA_LP_MODE_EN    (1<<13)
#define PMU_ANA_DSLEEP_ON          (1<<10)
#define PMU_ANA_VANA_DR          (1<<9)
#define PMU_ANA_VANA_EN          (1<<8)
#define PMU_ANA_DSLEEP_VOLT(n)          (((n) & 0xF) << 4)
#define PMU_ANA_DSLEEP_VOLT_MASK        (0xF << 4)
#define PMU_ANA_DSLEEP_VOLT_SHIFT       (4)
#define PMU_ANA_NORMAL_VOLT(n)           (((n) & 0xF) << 0)
#define PMU_ANA_NORMAL_VOLT_MASK         (0xF << 0)
#define PMU_ANA_NORMAL_VOLT_SHIFT        (0)

///49   dig setting
#define PMU_DIG_LP_MODE_EN    (1<<13)
#define PMU_DIG_DSLEEP_ON          (1<<10)
#define PMU_DIG_VDIG_DR          (1<<9)
#define PMU_DIG_VDIG_EN          (1<<8)
#define PMU_DIG_DSLEEP_VOLT(n)          (((n) & 0xF) << 4)
#define PMU_DIG_DSLEEP_VOLT_MASK        (0xF << 4)
#define PMU_DIG_DSLEEP_VOLT_SHIFT       (4)
#define PMU_DIG_NORMAL_VOLT(n)           (((n) & 0xF) << 0)
#define PMU_DIG_NORMAL_VOLT_MASK         (0xF << 0)
#define PMU_DIG_NORMAL_VOLT_SHIFT        (0)

///4A  IO SETTING
#define PMU_IO_LP_MODE_EN    (1<<15)
#define PMU_IO_DSLEEP_ON          (1<<12)
#define PMU_IO_VIO_DR          (1<<11)
#define PMU_IO_VIO_EN          (1<<10)
#define PMU_IO_NORMAL_VOLT(n)          (((n) & 0x1F) << 5)
#define PMU_IO_NORMAL_VOLT_MASK        (0x1F << 5)
#define PMU_IO_NORMAL_VOLT_SHIFT       (5)
#define PMU_IO_DSLEEP_VOLT(n)           (((n) & 0x1F) << 0)
#define PMU_IO_DSLEEP_VOLT_MASK         (0x1F << 0)
#define PMU_IO_DSLEEP_VOLT_SHIFT        (0)

///4B  VMEM SETTING
#define PMU_MEM_LP_MODE_EN    (1<<15)
#define PMU_MEM_DSLEEP_ON          (1<<12)
#define PMU_MEM_VMEM_DR          (1<<11)
#define PMU_MEM_VMEM_EN          (1<<10)
#define PMU_MEM_NORMAL_VOLT(n)          (((n) & 0x1F) << 5)
#define PMU_MEM_NORMAL_VOLT_MASK        (0x1F << 5)
#define PMU_MEM_NORMAL_VOLT_SHIFT       (5)
#define PMU_MEM_DSLEEP_VOLT(n)           (((n) & 0x1F) << 0)
#define PMU_MEM_DSLEEP_VOLT_MASK         (0x1F << 0)
#define PMU_MEM_DSLEEP_VOLT_SHIFT        (0)


////4C  GP SETTING
#define PMU_GP_LP_MODE_EN    (1<<15)
#define PMU_GP_DSLEEP_ON          (1<<12)
#define PMU_GP_VGP_DR          (1<<11)
#define PMU_GP_VGP_EN          (1<<10)
#define PMU_GP_NORMAL_VOLT(n)          (((n) & 0x1F) << 5)
#define PMU_GP_NORMAL_VOLT_MASK        (0x1F << 5)
#define PMU_GP_NORMAL_VOLT_SHIFT       (5)
#define PMU_GP_DSLEEP_VOLT(n)           (((n) & 0x1F) << 0)
#define PMU_GP_DSLEEP_VOLT_MASK         (0x1F << 0)
#define PMU_GP_DSLEEP_VOLT_SHIFT        (0)


////4D  USB SETTING
#define PMU_USB_LP_MODE_EN    (1<<13)
#define PMU_USB_DSLEEP_ON          (1<<10)
#define PMU_USB_VUSB_DR          (1<<9)
#define PMU_USB_VUSB_EN          (1<<8)
#define PMU_USB_NORMAL_VOLT(n)          (((n) & 0xF) << 4)
#define PMU_USB_NORMAL_VOLT_MASK        (0xF << 4)
#define PMU_USB_NORMAL_VOLT_SHIFT       (4)
#define PMU_USB_DSLEEP_VOLT(n)           (((n) & 0xF) << 0)
#define PMU_USB_DSLEEP_VOLT_MASK         (0xF << 0)
#define PMU_USB_DSLEEP_VOLT_SHIFT        (0)


////4E  CRYSTAL SETTING
#define PMU_CRYSTAL_LP_MODE_EN    (1<<10)
#define PMU_CRYSTAL_DSLEEP_ON          (1<<13)
#define PMU_CRYSTAL_VCRYSTAL_DR          (1<<12)
#define PMU_CRYSTAL_VCRYSTAL_EN          (1<<11)
#define PMU_CRYSTAL_NORMAL_VOLT(n)          (((n) & 0x7) << 4)
#define PMU_CRYSTAL_NORMAL_VOLT_MASK        (0x7 << 4)
#define PMU_CRYSTAL_NORMAL_VOLT_SHIFT       (3)
#define PMU_CRYSTAL_DSLEEP_VOLT(n)           (((n) & 0x7) << 0)
#define PMU_CRYSTAL_DSLEEP_VOLT_MASK         (0x7 << 0)
#define PMU_CRYSTAL_DSLEEP_VOLT_SHIFT        (0)



////4F  FM SETTING
#define PMU_FM_LP_MODE_EN    (1<<10)
#define PMU_FM_DSLEEP_ON          (1<<13)
#define PMU_FM_VFM_DR          (1<<12)
#define PMU_FM_VFM_EN          (1<<11)
#define PMU_FM_NORMAL_VOLT(n)          (((n) & 0x7) << 4)
#define PMU_FM_NORMAL_VOLT_MASK        (0x7 << 4)
#define PMU_FM_NORMAL_VOLT_SHIFT       (3)
#define PMU_FM_DSLEEP_VOLT(n)           (((n) & 0x7) << 0)
#define PMU_FM_DSLEEP_VOLT_MASK         (0x7 << 0)
#define PMU_FM_DSLEEP_VOLT_SHIFT        (0)



////50  PA SETTING
#define PMU_PA_VPA_EN          (1<<4)
#define PMU_PA_NORMAL_VOLT(n)           (((n) & 0x7) << 0)
#define PMU_PA_NORMAL_VOLT_MASK         (0x7 << 0)
#define PMU_PA_NORMAL_VOLT_SHIFT        (0)


////51  CODEC SETTING
#define PMU_CODEC_LP_MODE_EN    (1<<11)
#define PMU_CODEC_DSLEEP_ON          (1<<14)
#define PMU_CODEC_VCODEC_DR          (1<<13)
#define PMU_CODEC_VCODEC_EN          (1<<12)
#define PMU_CODEC_NORMAL_VOLT(n)          (((n) & 0xF) << 4)
#define PMU_CODEC_NORMAL_VOLT_MASK        (0xF << 4)
#define PMU_CODEC_NORMAL_VOLT_SHIFT       (4)
#define PMU_CODEC_DSLEEP_VOLT(n)           (((n) & 0xF) << 0)
#define PMU_CODEC_DSLEEP_VOLT_MASK         (0xF << 0)
#define PMU_CODEC_DSLEEP_VOLT_SHIFT        (0)

////56 BUCK VOLT SETTING
#define PMU_BUCK_VCORE_NORMAL(n)           (((n) & 0xF) << 12)
#define PMU_BUCK_VCORE_NORMAL_MASK         (0xF << 12)
#define PMU_BUCK_VCORE_NORMAL_SHIFT        (12)
#define PMU_BUCK_VCORE_DSLEEP(n)           (((n) & 0xF) << 8)
#define PMU_BUCK_VCORE_DSLEEP_MASK         (0xF << 8)
#define PMU_BUCK_VCORE_DSLEEP_SHIFT        (8)
#define PMU_BUCK_VANA_NORMAL(n)           (((n) & 0xF) << 4)
#define PMU_BUCK_VANA_NORMAL_MASK         (0xF << 4)
#define PMU_BUCK_VANA_NORMAL_SHIFT        (4)
#define PMU_BUCK_VANA_DSLEEP(n)           (((n) & 0xF) << 0)
#define PMU_BUCK_VANA_DSLEEP_MASK         (0xF << 0)
#define PMU_BUCK_VANA_DSLEEP_SHIFT        (0)

////57 DCDC SETTING
#define PMU_BUCK_CC_MODE_EN     (1<<12)
#define PMU_DCDC_ANA_LP_MODE_EN   (1<<11)
#define PMU_DCDC_ANA_DR   (1<<10)
#define PMU_DCDC_ANA_EN   (1<<9)
#define PMU_DCDC_ANA_DSLEEP_ON   (1<<6)

#define PMU_DCDC_DIG_LP_MODE_EN   (1<<5)
#define PMU_DCDC_DIG_DR   (1<<4)
#define PMU_DCDC_DIG_EN   (1<<3)
#define PMU_DCDC_DIG_DSLEEP_ON   (1<<0)


////67  PMU_REG_INT_MASK
#define PMU_INT_MASK_USB_INSERT         (1 << 15)

////68  PMU_REG_INT_EN
#define PMU_INT_EN_USB_INSERT           (1 << 15)

////69  PMU_REG_INT_STATUS
#define PMU_INT_STATUS_USB_INSERT       (1 << 15)

////6A  PMU_REG_INT_CLR
#define PMU_INT_CLR_USB_INSERT          (1 << 15)

////6C  PMU_REG_CHARGER_STATUS
#define PMU_USB_STATUS_RX_DP            (1 << 15)
#define PMU_USB_STATUS_RX_DP_SHIFT      (15)
#define PMU_USB_STATUS_RX_DM            (1 << 14)
#define PMU_USB_STATUS_RX_DM_SHIFT      (14)

////6D SLEEP ENABLE
#define PMU_SLEEP_EN    (1<<15)




#define PMU_DR(MOD)    PMU_##MOD##_V##MOD##_DR
#define PMU_LDO(MOD)    PMU_##MOD##_V##MOD##_EN
#define PMU_LP(MOD)    PMU_##MOD##_LP_MODE_EN
#define PMU_DSLP(MOD)    PMU_##MOD##_DSLEEP_ON
#define PMU_SLPV(MOD)    PMU_##MOD##_DSLEEP_VOLT_MASK
#define PMU_NORV(MOD)    PMU_##MOD##_NORMAL_VOLT_MASK
#define PMU_SLPV_SHFT(MOD)    PMU_##MOD##_DSLEEP_VOLT_SHIFT
#define PMU_NORV_SHFT(MOD)    PMU_##MOD##_NORMAL_VOLT_SHIFT





PMU_MODULE_CFG_T pmu_module_cfg[]=
{
{PMU_ANA,PMU_DR(ANA),PMU_LDO(ANA),PMU_LP(ANA),PMU_DSLP(ANA),PMU_SLPV(ANA),PMU_SLPV_SHFT(ANA),PMU_NORV(ANA),PMU_NORV_SHFT(ANA)},
{PMU_DIG,PMU_DR(DIG),PMU_LDO(DIG),PMU_LP(DIG),PMU_DSLP(DIG),PMU_SLPV(DIG),PMU_SLPV_SHFT(DIG),PMU_NORV(DIG),PMU_NORV_SHFT(DIG)},
{PMU_IO,PMU_DR(IO),PMU_LDO(IO),PMU_LP(IO),PMU_DSLP(IO),PMU_SLPV(IO),PMU_SLPV_SHFT(IO),PMU_NORV(IO),PMU_NORV_SHFT(IO)},
{PMU_MEM,PMU_DR(MEM),PMU_LDO(MEM),PMU_LP(MEM),PMU_DSLP(MEM),PMU_SLPV(MEM),PMU_SLPV_SHFT(MEM),PMU_NORV(MEM),PMU_NORV_SHFT(MEM)},
{PMU_GP,PMU_DR(GP),PMU_LDO(GP),PMU_LP(GP),PMU_DSLP(GP),PMU_SLPV(GP),PMU_SLPV_SHFT(GP),PMU_NORV(GP),PMU_NORV_SHFT(GP)},
{PMU_USB,PMU_DR(USB),PMU_LDO(USB),PMU_LP(USB),PMU_DSLP(USB),PMU_SLPV(USB),PMU_SLPV_SHFT(USB),PMU_NORV(USB),PMU_NORV_SHFT(USB)},
{PMU_CRYSTAL,PMU_DR(CRYSTAL),PMU_LDO(CRYSTAL),PMU_LP(CRYSTAL),PMU_DSLP(CRYSTAL),PMU_SLPV(CRYSTAL),PMU_SLPV_SHFT(CRYSTAL),PMU_NORV(CRYSTAL),PMU_NORV_SHFT(CRYSTAL)},
{PMU_FM,PMU_DR(FM),PMU_LDO(FM),PMU_LP(FM),PMU_DSLP(FM),PMU_SLPV(FM),PMU_SLPV_SHFT(FM),PMU_NORV(FM),PMU_NORV_SHFT(FM)},
{PMU_PA,0,PMU_LDO(ANA),0,0,0,0,PMU_NORV(PA),PMU_NORV_SHFT(PA)},
{PMU_CODEC,PMU_DR(CODEC),PMU_LDO(CODEC),PMU_LP(CODEC),PMU_DSLP(CODEC),PMU_SLPV(CODEC),PMU_SLPV_SHFT(CODEC),PMU_NORV(CODEC),PMU_NORV_SHFT(CODEC)},

};



enum PMU_REG_T {
    PMU_REG_POWER_OFF = 0x40,
    PMU_REG_USB_PIN_CFG = 0x45,
    PMU_REG_MODULE_START  = 0x48,
    PMU_REG_ANA_CFG  = 0x48,
    PMU_REG_DIG_CFG  = 0x49,
    PMU_REG_IO_CFG  = 0x4A,
    PMU_REG_MEM_CFG    = 0x4B,
    PMU_REG_GP_CFG   =0x4C,
    PMU_REG_USB_CFG  = 0x4D,
    PMU_REG_CRYSTAL_CFG  = 0x4E,
    PMU_REG_FM_CFG  = 0x4F,
    PMU_REG_PA_CFG  = 0x50,
    PMU_REG_CODEC_CFG  = 0x51,
    PMU_REG_BUCK_VOLT = 0x56,
    PMU_REG_DCDC_CFG  = 0x57,
    PMU_REG_INT_MASK = 0x67,
    PMU_REG_INT_EN = 0x68,
    PMU_REG_INT_STATUS = 0x69,
    PMU_REG_INT_CLR = 0x6A,
    PMU_REG_CHARGER_CFG = 0x6B,
    PMU_REG_CHARGER_STATUS = 0x6C,
    PMU_REG_SLEEP_CFG = 0x6D,
    PMU_REG_EFUSE_SEL = 0x6F,
    PMU_REG_EFUSE_VAL = 0x70,
    PMU_REG_VANA_HIGH_VAL = 0x78,
};

static uint8_t pmu_usb_enabled;
enum PMU_USB_PIN_CHK_STATUS_T usb_pin_status;

PMU_USB_PIN_CHK_CALLBACK usb_pin_callback;

#if 0   ///0.9 dig dcdc
int pmu_open(void)
{
    int ret;

    ret = ana_open();
    if (ret) {
        return ret;
    }

    // Init for metal 0
    // Bypass PLL lock check
    *(volatile unsigned int *)(0x40000060) |= (1 << 23) | (1 << 14) | (1 << 5);
    // PMU settings
    pmu_write(0x4b, 0xb6c6); // increase vmem volatge from 1.8v to 3v
    pmu_write(0x45, 0x645A); // set usb_pu_rx_se
    //pmu_write(0x47, 0x1A61); // default usb tx io drive (2)
    //pmu_write(0x47, 0x1661); // reduce usb tx io drive (1)
    pmu_write(0x47, 0x1E61); // increase usb tx io drive (3)
    //pmu_write(0x46, 0x01E0); // decrease pull-up resistor
    pmu_write(0x4D, 0x29C9); // increase ldo voltage for USB host

 //   pmu_write(0x34, 0xa000); // disable pll

    // Allow sleep
    pmu_write(0x6D, 0x9700);

    pmu_write(0x4E, 0x0DA4); // disbale 26m
    pmu_write(0x4B, 0xAaC6); // vmem disable
    pmu_write(0x57, 0x0640); // dcdc enable
    pmu_write(0x48, 0x2A66); // ana ldo disable
    pmu_write(0x57, 0x0FC0); // vdcdc
    pmu_write(0x49, 0x2D3A); //vdig
    pmu_write(0x56, 0x34A2); // vdcdc

#if 1
    pmu_write(0x4c, 0xaa10); // gpldo
    pmu_write(0x51, 0x2a55); // vc0dec
    pmu_write(0x57, 0x0e00); //vdcdc
    pmu_write(0x48, 0x2e66); // vana
    pmu_write(0x4d,0x2ac9);//vusb
    pmu_write(0x4f,0x1124);//vfm
    pmu_write(0x4a,0xaa10);//vvio
#if 1////dcdc sido
    pmu_write(0x57,0x1618);//vvio
    pmu_write(0x49,0x2eaa);//vvio
    pmu_write(0x56,0x14A2);//VCORE =0.9V
#endif

//    pmu_write(0x4a,0xaa10);//vvio

#endif


    return 0;
}
#endif


uint32_t read_hw_metal_id(void)
{
    int ret;
    uint16_t val;

    ret = pmu_read(PMU_REG_POWER_OFF, &val);
    if (ret) {
        return 0;
    }

    return GET_BITFIELD(val, PMU_METAL_ID);
}

int pmu_charger_detected(void)
{
    int ret;
    unsigned short charger;

    ret = pmu_read(PMU_REG_CHARGER_STATUS, &charger);
    if (ret) {
        return 0;
    }

    return !!(charger & PMU_CHARGER_STATUS_IN);
}

int pmu_get_efuse(enum PMU_EFUSE_PAGE_T page, unsigned short *efuse)
{
    int ret;
    unsigned short val;
    volatile int i;

    val = PMU_EFUSE_CLK_EN;
    ret = pmu_write(PMU_REG_EFUSE_SEL, val);
    if (ret) {
        return ret;
    }

    val = PMU_EFUSE_TRIGGER | PMU_EFUSE_CLK_EN |
          PMU_EFUSE_PROG_TIME(4) | PMU_EFUSE_WORD_SEL(page);
    ret = pmu_write(PMU_REG_EFUSE_SEL, val);
    if (ret) {
        return ret;
    }

    // Delay 10 us
    for (i = 0; i < 100; i++);

    ret = pmu_read(PMU_REG_EFUSE_VAL, efuse);
    if (ret) {
        return ret;
    }

    return 0;
}

static unsigned int pmu_count_zeros(unsigned int val, unsigned int bits)
{
    int cnt = 0;
    int i;

    for (i = 0; i < bits; i++) {
        if ((val & (1 << i)) == 0) {
            cnt++;
        }
    }

    return cnt;
}

int pmu_get_security_value(union SECURITY_VALUE_T *val)
{
    int ret;

    ret = pmu_get_efuse(PMU_EFUSE_PAGE_SECURITY, &val->reg);
    if (ret) {
        // Error
        goto _no_security;
    }

    if (!val->security_en) {
        // OK
        goto _no_security;
    }
    ret = 1;
    if (pmu_count_zeros(val->key_id, 3) != val->key_chksum) {
        // Error
        goto _no_security;
    }
    if (pmu_count_zeros(val->vendor_id, 6) != val->vendor_chksum) {
        // Error
        goto _no_security;
    }
    if ((pmu_count_zeros(val->reg, 15) & 1) != val->chksum) {
        // Error
        goto _no_security;
    }

    // OK
    return 0;

_no_security:
    val->reg = 0;

    return ret;
}

#if defined(PROGRAMMER)
void pmu_shutdown(void)
{
	unsigned short val;

    uint32_t lock = int_lock();

	hal_sys_timer_delay(MS_TO_TICKS(1));

    //set pmu reset
	hal_analogif_reg_read(0x40,&val);
	val |=0x1;
	hal_analogif_reg_write(0x40,val);
	hal_sys_timer_delay(MS_TO_TICKS(1));

    //set lpo off delay
	hal_analogif_reg_write(0x41,0xAAF1);

	//clr reset ,set sw power down
	val |=0x2;
	val &=0xfffe;
	hal_analogif_reg_write(0x40,val);
	hal_sys_timer_delay(MS_TO_TICKS(1));
	hal_analogif_reg_write(0x40,val);
	hal_sys_timer_delay(MS_TO_TICKS(2000));

    int_unlock(lock);
	//can't reach here
	ASSERT(0, "shutdown failed");
	while(1);
}
#else
void pmu_shutdown(void)
{
	unsigned short val_40h;
	unsigned short val_49h;
    uint32_t lock = int_lock();

	hal_sleep_set_unsleep_lock(HAL_SLEEP_UNSLEEP_LOCK_APP);

	pmu_mode_change(BEST1000_LDO_MODE);
	hal_sys_timer_delay(MS_TO_TICKS(1));

    //set pmu reset
	hal_analogif_reg_read(0x40,&val_40h);
	val_40h |=0x1;
	TRACE("POWER DONW =%x",val_40h);
	hal_analogif_reg_write(0x40,val_40h);
	hal_sys_timer_delay(MS_TO_TICKS(1));

    //set pmu codec power off config

	//set pmu power off config
	//reduce popclick next power on
	hal_analogif_reg_read(PMU_REG_DIG_CFG,&val_49h);
    val_49h |= (1<<8)|(1<<9);
	hal_analogif_reg_write(PMU_REG_DIG_CFG,val_49h);

    //set lpo off delay
	hal_analogif_reg_write(0x41,0xAAF1);
    //set reg_ac_on_db_value (0)
	hal_analogif_reg_write(0x6b,0x1F00);

	//clr reset ,set sw power down
	val_40h |=0x2;
	val_40h &=0xfffe;
	TRACE("POWER DONW =%x",val_40h);
	hal_analogif_reg_write(0x40,val_40h);
	hal_sys_timer_delay(MS_TO_TICKS(1));
	hal_analogif_reg_write(0x40,val_40h);
	hal_sys_timer_delay(MS_TO_TICKS(2000));

    int_unlock(lock);
	//can't reach here
	ASSERT(0, "shutdown failed");
	while(1);
}
#endif

void pmu_module_config(enum PMU_MODUAL_T module,unsigned short is_manual,unsigned short ldo_on,unsigned short lp_mode,unsigned short dpmode)
{
    int ret;
    unsigned short val;

    unsigned char module_address;
    PMU_MODULE_CFG_T *module_cfg_p = &pmu_module_cfg[module];

    module_address = module+PMU_REG_MODULE_START;
    ret = pmu_read(module_address, &val);
    if(ret == 0)
    {
        if(is_manual)
        {
            val |= module_cfg_p->manual_bit;
        }
        else
        {
            val &= ~module_cfg_p->manual_bit;
        }
        if (PMU_FM == module){
            ldo_on = ldo_on > 0 ? PMU_LDO_OFF : PMU_LDO_ON;
        }
        if(ldo_on)
        {
            val |=module_cfg_p->ldo_en;
        }
        else
        {
            val &= ~module_cfg_p->ldo_en;
        }
        if(lp_mode)
        {
            val |=module_cfg_p->lp_mode;
        }
        else
        {
            val &= ~module_cfg_p->lp_mode;
        }
        if(dpmode)
        {
            val |=module_cfg_p->dsleep_mode;
        }
        else
        {
            val &= ~module_cfg_p->dsleep_mode;
        }
        pmu_write(module_address, val);

    }


}


void pmu_module_set_volt(unsigned char module,unsigned short  sleep_v,unsigned short normal_v)
{
    int ret;
    unsigned short val;

    unsigned char module_address;
    PMU_MODULE_CFG_T *module_cfg_p = &pmu_module_cfg[module];

    module_address = module+PMU_REG_MODULE_START;
    ret = pmu_read(module_address, &val);
    if(ret == 0)
    {
        val &= ~module_cfg_p->normal_v;

        val |= (normal_v<<module_cfg_p->normal_v_shift);
        val &= ~module_cfg_p->dsleep_v;
        val |= sleep_v<<module_cfg_p->dsleep_v_shift;

        pmu_write(module_address, val);
    }
}


void pmu_ldo_mode_en(unsigned char en)
{
    if(en)
    {
        pmu_write(PMU_REG_DCDC_CFG, 0);
    }
}


/////dcdc mode just have the normal mode  so just set the buck vcore vana normal voltage
void pmu_dcdc_ana_set_volt(unsigned short normal_v,unsigned short dsleep_v)
{
    int ret;
    unsigned short val;

    ret = pmu_read(PMU_REG_BUCK_VOLT, &val);
    if(ret == 0)
    {
        val &= ~PMU_BUCK_VANA_DSLEEP_MASK;
        val &= ~PMU_BUCK_VANA_NORMAL_MASK;
        val |= PMU_BUCK_VANA_DSLEEP(dsleep_v);
        val |= PMU_BUCK_VANA_NORMAL(normal_v);
        pmu_write(PMU_REG_BUCK_VOLT, val);
    }

}

void pmu_dcdc_ana_get_volt(unsigned short *normal_v,unsigned short *dsleep_v)
{
    int ret;
    unsigned short val;

    ret = pmu_read(PMU_REG_BUCK_VOLT, &val);
    if(ret == 0)
    {
        *normal_v = (val&PMU_BUCK_VANA_NORMAL_MASK)>>PMU_BUCK_VANA_NORMAL_SHIFT;
        *dsleep_v = (val&PMU_BUCK_VANA_DSLEEP_MASK)>>PMU_BUCK_VANA_DSLEEP_SHIFT;
    }
}



void pmu_dcdc_ana_mode_en(unsigned char en)
{
    int ret;
    unsigned short val;


    ret = pmu_read(PMU_REG_DCDC_CFG, &val);
    if(ret == 0)
    {
        if(en)
        {

            val |= PMU_DCDC_ANA_LP_MODE_EN;
            val |=PMU_DCDC_ANA_DR;
            val |=PMU_DCDC_ANA_EN;
            pmu_write(PMU_REG_DCDC_CFG, val);

        }
        else
        {
          //disable dcdc ana mode should turn on the vana  and then disable dcdc mode
            ////enable vana ldo
            pmu_module_config(PMU_ANA,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);

            pmu_ldo_mode_en(1);


        }
    }

}





/////dcdc mode just have the normal mode  so just set the buck vcore vana normal voltage
void pmu_dcdc_dig_set_volt(unsigned short normal_v,unsigned short dsleep_v)
{
    int ret;
    unsigned short val;

    ret = pmu_read(PMU_REG_BUCK_VOLT, &val);
    if(ret == 0)
    {
        val &= ~PMU_BUCK_VCORE_NORMAL_MASK;
        val &= ~PMU_BUCK_VCORE_DSLEEP_MASK;
        val |= PMU_BUCK_VCORE_NORMAL(normal_v);
        val |= PMU_BUCK_VCORE_DSLEEP(dsleep_v);
        pmu_write(PMU_REG_BUCK_VOLT, val);
    }

}

static void pmu_dig_set_volt(int mode_change, unsigned char mode)
{
    uint32_t lock;

    lock = int_lock();
    if (0) {
        // ANC needs 1.3V or above vcore
        if (mode_change == 0) {
            pmu_module_set_volt(PMU_DIG,PMU_VDIG_0_7V,PMU_VDIG_1_3V);
            pmu_dcdc_dig_set_volt(PMU_DCDC_DIG_1_3V,PMU_DCDC_DIG_0_8V);
        }
    } else if (pmu_usb_enabled ) {
        // USB/FLASH_WRITE needs 1.1V or above vcore
        if (mode_change == 0) {
            pmu_module_set_volt(PMU_DIG,PMU_VDIG_0_7V,PMU_VDIG_1_1V);
            pmu_dcdc_dig_set_volt(PMU_DCDC_DIG_1_1V,PMU_DCDC_DIG_0_8V);
        }
    } else {
        // Common cases
        if (mode == BEST1000_LDO_MODE) {
            pmu_module_set_volt(PMU_DIG,PMU_VDIG_1_1V,PMU_VDIG_1_1V);
        } else if (mode == BEST1000_ANA_DCDC_MODE) {
            pmu_module_set_volt(PMU_DIG,PMU_VDIG_0_7V,PMU_VDIG_0_9V);
        } else if (mode == BEST1000_DIG_DCDC_MODE) {
            pmu_dcdc_dig_set_volt(PMU_DCDC_DIG_0_9V,PMU_DCDC_DIG_0_8V);
        }
    }
    int_unlock(lock);
}

void pmu_dcdc_dual_mode_en(unsigned char en)
{
    int ret;
    unsigned short val;

    ret = pmu_read(PMU_REG_DCDC_CFG, &val);
    if(ret == 0)
    {
        if(en)
        {
            ////dcdc mode have no lp mode so just open the dcdc dig and dcdc ana and close the vdig and vana
            val |= PMU_BUCK_CC_MODE_EN;
            val |=PMU_DCDC_DIG_DR;
            val |=PMU_DCDC_DIG_EN;
            val |=PMU_DCDC_ANA_DR;
            val |=PMU_DCDC_ANA_EN;
            pmu_write(PMU_REG_DCDC_CFG, val);
            //disable ldo mode should after dcdc mode enable, then pmu can shutdown the vana and vdig
            pmu_module_config(PMU_ANA,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);
            pmu_module_config(PMU_DIG,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);

        }
        else
        {
            //disable dcdc dual mode should turn on the vana and vdig and then disable dcdc mode
            ////enable vana ldo
            pmu_module_config(PMU_ANA,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);
            ///enable vdig ldo
            pmu_module_config(PMU_DIG,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);

            pmu_ldo_mode_en(1);
        }

    }

}



void pmu_sleep_en(unsigned char sleep_en)
{
    int ret;
    unsigned short val;

    ret = pmu_read(PMU_REG_SLEEP_CFG, &val);
    if(ret == 0)
    {
        if(sleep_en)
        {
            val |=PMU_SLEEP_EN;
        }
        else
        {
            val &= ~PMU_SLEEP_EN;
        }
        pmu_write(PMU_REG_SLEEP_CFG, val);
    }

}

unsigned char pmu_active_mode;

//#define LDO_MODE
#define ANA_DCDC_MODE
//#define DIG_DCDC_MODE
//#define ANA_HIGH_VOLT

#ifdef ANA_HIGH_VOLT
void pmu_vana_en(unsigned char en)
{
    int ret;
    unsigned short val;

    ret = pmu_read(PMU_REG_VANA_HIGH_VAL, &val);
    if(ret == 0)
    {
        if(en)
        {
            val |=0x4;
        }
        else
        {
            val &= ~0x4;
        }
        pmu_write(PMU_REG_VANA_HIGH_VAL, val);
    }
}
#endif

#if defined(PROGRAMMER)
int pmu_open(void)
{
    // Init for metal 0
    // Bypass PLL lock check
    *(volatile unsigned int *)(0x40000060) |= (1 << 23) | (1 << 14) | (1 << 5);

    pmu_sleep_en(0);  //enable sleep

    return 0;
}
#else
int pmu_open(void)
{
	unsigned short val_49h;
    // Init for metal 0
    // Bypass PLL lock check
    *(volatile unsigned int *)(0x40000060) |= (1 << 23) | (1 << 14) | (1 << 5);

    // PMU settings
//    pmu_module_set_volt(PMU_MEM,PMU_VMEM_2V,PMU_VMEM_3V); // increase vmem volatge from 1.8v to 3v
//    pmu_write(0x45, 0x645A); // set usb_pu_rx_se
    //pmu_write(0x47, 0x1A61); // default usb tx io drive (2)
    //pmu_write(0x47, 0x1661); // reduce usb tx io drive (1)
//    pmu_write(0x47, 0x1E61); // increase usb tx io drive (3)
    //pmu_write(0x46, 0x01E0); // decrease pull-up resistor
 //   pmu_module_set_volt(PMU_USB,9,0xc); ///increase vusb voltage
 //   pmu_write(0x34, 0xa000); // disable pll

#ifndef __FORCE_DISABLE_SLEEP__
    pmu_sleep_en(1);  //enable sleep
#endif

	//resume dig setting (setting by power shundown)
	hal_analogif_reg_read(PMU_REG_DIG_CFG,&val_49h);
    val_49h &= ~((1<<8)|(1<<9));
	hal_analogif_reg_write(PMU_REG_DIG_CFG,val_49h);
#ifdef ANA_HIGH_VOLT
     pmu_vana_en(1);
#endif
    pmu_module_config(PMU_CRYSTAL,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable 26m when sleep
#if 0
    pmu_module_config(PMU_MEM,PMU_MANUAL_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON); //disable VMEM
#else
    pmu_module_config(PMU_MEM,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VMEM
#endif
    pmu_module_config(PMU_GP,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VGP
    pmu_module_config(PMU_USB,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VUSB
    /*pmu_module_config(PMU_FM,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_OFF,PMU_DSLEEP_MODE_OFF); *///disable VFM
    //pmu_module_config(PMU_IO,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VIO
#ifndef __BEST_FLASH_VIA_ANA__
    pmu_module_config(PMU_CODEC,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VCODEC
    pmu_module_config(PMU_IO,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON); //disable VIO
#else
    pmu_module_config(PMU_IO,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VIO
#endif

    pmu_module_set_volt(PMU_IO,PMU_IO_2_7V,PMU_IO_2_6V);


#ifdef LDO_MODE
    pmu_module_set_volt(PMU_DIG,PMU_VDIG_0_7V,PMU_VDIG_1_1V);//set vcore normal 1.1v vcore sleep 0.7v
    pmu_module_config(PMU_ANA,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);
    pmu_active_mode = BEST1000_LDO_MODE;
#endif

#ifdef ANA_DCDC_MODE

    pmu_dcdc_ana_mode_en(1);  //enable ana dcdc

    pmu_module_config(PMU_ANA,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable Vana
#ifdef _BEST1000_QUAL_DCDC_
#ifndef __BEST_FLASH_VIA_ANA__    
#ifdef ANA_HIGH_VOLT
    pmu_dcdc_ana_set_volt(PMU_HIGH_DCDC_ANA_1_7V,PMU_HIGH_DCDC_ANA_SLEEP_2_0V);
#else
    pmu_dcdc_ana_set_volt(PMU_DCDC_ANA_2_0V,PMU_DCDC_ANA_SLEEP_1_6V);
#endif
#else
#ifdef ANA_HIGH_VOLT
    pmu_dcdc_ana_set_volt(PMU_HIGH_DCDC_ANA_1_7V,PMU_HIGH_DCDC_ANA_SLEEP_2_0V);
#else
    pmu_dcdc_ana_set_volt(PMU_DCDC_ANA_2_2V,PMU_DCDC_ANA_SLEEP_1_6V);
#endif
#endif
#else
#ifdef ANA_HIGH_VOLT
    pmu_dcdc_ana_set_volt(PMU_HIGH_DCDC_ANA_1_7V,PMU_HIGH_DCDC_ANA_SLEEP_2_0V);
#else
    pmu_dcdc_ana_set_volt(PMU_DCDC_ANA_1_8V,PMU_DCDC_ANA_SLEEP_1_6V);
#endif
#endif
    pmu_module_set_volt(PMU_DIG,PMU_VDIG_0_7V,PMU_VDIG_0_9V);//set vcore normal 1.1v vcore sleep 0.7v

    pmu_active_mode = BEST1000_ANA_DCDC_MODE;
#endif

#ifdef  DIG_DCDC_MODE
    pmu_dcdc_dual_mode_en(1);
    pmu_module_config(PMU_ANA,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable Vana
    pmu_module_config(PMU_DIG,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable Vana
#ifdef ANA_HIGH_VOLT
    pmu_dcdc_ana_set_volt(PMU_HIGH_DCDC_ANA_1_7V,PMU_HIGH_DCDC_ANA_SLEEP_2_0V);
#else    
    pmu_dcdc_ana_set_volt(PMU_DCDC_ANA_1_8V,PMU_DCDC_ANA_SLEEP_1_6V);
#endif
    pmu_dcdc_dig_set_volt(PMU_DCDC_DIG_0_9V,PMU_DCDC_DIG_0_8V);

    pmu_active_mode = BEST1000_DIG_DCDC_MODE;
#endif
#ifdef __BEST_FLASH_VIA_ANA__
    pmu_module_config(PMU_CODEC,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VCODEC
#endif
    return 0;
}
#endif

void pmu_mode_change(unsigned char mode)
{
    uint32_t lock;

    if (pmu_active_mode == mode)
        return;

    lock = int_lock();

    ////enable vana ldo
    pmu_module_config(PMU_ANA,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);
    ///enable vdig ldo
    pmu_module_config(PMU_DIG,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);

    pmu_ldo_mode_en(1);
    pmu_module_set_volt(PMU_DIG,PMU_VDIG_1_1V,PMU_VDIG_1_1V);//set vcore normal 1.1v vcore sleep 0.7v

    if(mode == BEST1000_ANA_DCDC_MODE)
    {
        pmu_dcdc_ana_mode_en(1);  //enable ana dcdc

        pmu_module_config(PMU_ANA,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable Vana
#ifdef _BEST1000_QUAL_DCDC_
        if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
        {
            pmu_dcdc_ana_set_volt(PMU_DCDC_ANA_1_8V,PMU_DCDC_ANA_SLEEP_1_6V);
        }
        else
        {
#ifndef __BEST_FLASH_VIA_ANA__    
#ifdef ANA_HIGH_VOLT
    pmu_dcdc_ana_set_volt(PMU_HIGH_DCDC_ANA_1_7V,PMU_HIGH_DCDC_ANA_SLEEP_2_0V);
#else
            pmu_dcdc_ana_set_volt(PMU_DCDC_ANA_2_0V,PMU_DCDC_ANA_SLEEP_1_6V);
#endif
#else
        pmu_dcdc_ana_set_volt(PMU_DCDC_ANA_2_2V,PMU_DCDC_ANA_SLEEP_1_6V);
#endif
//           pmu_dcdc_ana_set_volt(PMU_DCDC_ANA_1_8V,PMU_DCDC_ANA_SLEEP_1_6V);
        }
#endif
        pmu_module_set_volt(PMU_DIG,PMU_VDIG_0_7V,PMU_VDIG_0_9V);//set vcore normal 1.1v vcore sleep 0.7v

    }
    else if(mode == BEST1000_DIG_DCDC_MODE)
    {
        pmu_dcdc_dual_mode_en(1);
        pmu_module_config(PMU_ANA,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable Vana
        pmu_module_config(PMU_DIG,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable Vana
#ifdef _BEST1000_QUAL_DCDC_
        if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
        {
            pmu_dcdc_ana_set_volt(PMU_DCDC_ANA_1_8V,PMU_DCDC_ANA_SLEEP_1_6V);
        }
        else
        {
#ifdef ANA_HIGH_VOLT
    		pmu_dcdc_ana_set_volt(PMU_HIGH_DCDC_ANA_1_7V,PMU_HIGH_DCDC_ANA_SLEEP_2_0V);
#else
        	pmu_dcdc_ana_set_volt(PMU_DCDC_ANA_1_8V,PMU_DCDC_ANA_SLEEP_1_6V);
#endif
        }
#endif
        pmu_dcdc_dig_set_volt(PMU_DCDC_DIG_0_9V,PMU_DCDC_DIG_0_8V);
    }

    pmu_active_mode = mode;

    int_unlock(lock);
}

void pmu_flash_write_config(void)
{
#ifdef __BEST_FLASH_VIA_ANA__
    pmu_module_config(PMU_CODEC,PMU_MANUAL_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VCODEC
#endif
    if (pmu_active_mode == BEST1000_LDO_MODE){
        pmu_module_set_volt(PMU_DIG,PMU_VDIG_1_1V,PMU_VDIG_1_1V);//set vcore normal 1.1v vcore sleep 0.7v
    }else if (pmu_active_mode == BEST1000_ANA_DCDC_MODE){
        pmu_module_set_volt(PMU_DIG,PMU_VDIG_1_1V,PMU_VDIG_1_1V);//set vcore normal 1.1v vcore sleep 0.7v
    }else if (pmu_active_mode == BEST1000_DIG_DCDC_MODE){
        pmu_dcdc_dig_set_volt(PMU_DCDC_DIG_1_1V,PMU_DCDC_DIG_1_1V);
    }
}

void pmu_flash_read_config(void)
{
    if (pmu_active_mode == BEST1000_LDO_MODE){
        pmu_module_set_volt(PMU_DIG,PMU_VDIG_1_1V,PMU_VDIG_1_1V);//set vcore normal 1.1v vcore sleep 0.7v
    }else if (pmu_active_mode == BEST1000_ANA_DCDC_MODE){
        pmu_module_set_volt(PMU_DIG,PMU_VDIG_0_7V,PMU_VDIG_0_9V);//set vcore normal 1.1v vcore sleep 0.7v
    }else if (pmu_active_mode == BEST1000_DIG_DCDC_MODE){
        pmu_dcdc_dig_set_volt(PMU_DCDC_DIG_0_9V,PMU_DCDC_DIG_0_8V);
    }
#ifdef __BEST_FLASH_VIA_ANA__
    pmu_module_config(PMU_CODEC,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VCODEC
#endif
}

void pmu_anc_config(int enable)
{
    pmu_dig_set_volt(0, 0);
}

void pmu_usb_config(int enable)
{
    unsigned short ldo_on;

    if (enable) {
        ldo_on = PMU_LDO_ON;
    } else {
        ldo_on = PMU_LDO_OFF;
    }
    pmu_module_config(PMU_USB,PMU_MANUAL_MODE,ldo_on,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);

    pmu_module_set_volt(PMU_DIG,PMU_VDIG_0_7V,PMU_VDIG_1_1V); // Vcore needs 1.0V(BGA)/1.1V(QFN) or above for usbhost

    // PMU settings

    // Only for metal 0
    //pmu_write(0x45, 0x645A); // set usb_pu_rx_se

    //pmu_write(0x4b, 0xb6c6); // increase vmem volatge from 1.8v to 3v
    //pmu_write(0x47, 0x1A61); // default usb tx io drive (2)
    //pmu_write(0x47, 0x1661); // reduce usb tx io drive (1)
    //pmu_write(0x47, 0x1E61); // increase usb tx io drive (3)

    //pmu_write(0x46, 0x01E0); // decrease pull-up resistor

    //pmu_write(0x4D, 0x29C9); // increase ldo voltage for USB host
}

static void pmu_usb_pin_irq_handler(void)
{
    int dp, dn;

    pmu_write(PMU_REG_INT_CLR, PMU_INT_CLR_USB_INSERT);

    if (usb_pin_callback) {
        pmu_usb_get_pin_status(&dp, &dn);

        if ( (usb_pin_status == PMU_USB_PIN_CHK_DEV_CONN && (dp == 1 && dn == 0)) ||
                (usb_pin_status == PMU_USB_PIN_CHK_DEV_DISCONN && (dp == 0 && dn == 0)) ||
                (usb_pin_status == PMU_USB_PIN_CHK_HOST_RESUME && dp == 0) ) {
            // HOST_RESUME: (resume) dp == 0 && dn == 1, (reset) dp == 0 && dn == 0
            pmu_usb_disable_pin_status_check();
            usb_pin_callback(usb_pin_status);
        }
    }
}

int pmu_usb_enable_pin_status_check(enum PMU_USB_PIN_CHK_STATUS_T status, PMU_USB_PIN_CHK_CALLBACK callback)
{
    uint16_t val;
    uint32_t lock;
    int dp, dn;

    if (status >= PMU_USB_PIN_CHK_STATUS_QTY) {
        return 1;
    }

    NVIC_DisableIRQ(PMU_USB_IRQn);

    lock = int_lock();

    usb_pin_status = status;
    usb_pin_callback = callback;

    // Mask the irq
    pmu_read(PMU_REG_INT_MASK, &val);
    val &= ~PMU_INT_MASK_USB_INSERT;
    pmu_write(PMU_REG_INT_MASK, val);

    // Config pin check
    pmu_read(PMU_REG_USB_PIN_CFG, &val);
    val &= ~(PMU_USB_POL_RX_DP | PMU_USB_POL_RX_DM);
    val |= PMU_USB_DEBOUNCE_EN | PMU_USB_NOLS_MODE | PMU_USB_INSERT_DET_EN;
    if (status == PMU_USB_PIN_CHK_DEV_CONN) {
        // Check dp 0->1
    } else if (status == PMU_USB_PIN_CHK_DEV_DISCONN) {
        // Check dp 1->0
        val |= PMU_USB_POL_RX_DP;
    } else if (status == PMU_USB_PIN_CHK_HOST_RESUME) {
        // Check dp 1->0, dm 0->1
        val |= PMU_USB_POL_RX_DP | PMU_USB_POL_RX_DM;
    }
    pmu_write(PMU_REG_USB_PIN_CFG, val);

    if (status != PMU_USB_PIN_CHK_NONE && callback) {
        pmu_read(PMU_REG_INT_EN, &val);
        val |= PMU_INT_EN_USB_INSERT;
        pmu_write(PMU_REG_INT_EN, val);

        pmu_read(PMU_REG_INT_MASK, &val);
        val |= PMU_INT_MASK_USB_INSERT;
        pmu_write(PMU_REG_INT_MASK, val);

        pmu_write(PMU_REG_INT_CLR, PMU_INT_CLR_USB_INSERT);
    }

    int_unlock(lock);

    if (status != PMU_USB_PIN_CHK_NONE && callback) {
        // Wait 6 cycles of 32K clock for the new status when signal checking polarity is changed
        hal_sys_timer_delay(4);
        pmu_write(PMU_REG_INT_CLR, PMU_INT_CLR_USB_INSERT);
        NVIC_ClearPendingIRQ(PMU_USB_IRQn);

        pmu_usb_get_pin_status(&dp, &dn);
        if ( (status == PMU_USB_PIN_CHK_DEV_CONN && (dp == 1 && dn == 0)) ||
                (status == PMU_USB_PIN_CHK_DEV_DISCONN && (dp == 0 && dn == 0)) ||
                (status == PMU_USB_PIN_CHK_HOST_RESUME && (dp == 0 && dn == 1)) ) {
            pmu_usb_disable_pin_status_check();
            callback(status);
            return 0;
        }

        NVIC_SetVector(PMU_USB_IRQn, (uint32_t)pmu_usb_pin_irq_handler);
        NVIC_SetPriority(PMU_USB_IRQn, IRQ_PRIORITY_NORMAL);
        NVIC_EnableIRQ(PMU_USB_IRQn);
    }

    return 0;
}

void pmu_usb_disable_pin_status_check(void)
{
    uint16_t val;
    uint32_t lock;

    NVIC_DisableIRQ(PMU_USB_IRQn);

    lock = int_lock();

    pmu_read(PMU_REG_INT_EN, &val);
    val &= ~PMU_INT_EN_USB_INSERT;
    pmu_write(PMU_REG_INT_EN, val);

    pmu_read(PMU_REG_USB_PIN_CFG, &val);
    val &= ~PMU_USB_INSERT_DET_EN;
    pmu_write(PMU_REG_USB_PIN_CFG, val);

    int_unlock(lock);
}

void pmu_usb_get_pin_status(int *dp, int *dn)
{
    uint16_t pol, val;

    pmu_read(PMU_REG_USB_PIN_CFG, &pol);
    pmu_read(PMU_REG_CHARGER_STATUS, &val);

    *dp = ((pol & PMU_USB_POL_RX_DP) >> PMU_USB_POL_RX_DP_SHIFT) ^
        ((val & PMU_USB_STATUS_RX_DP) >> PMU_USB_STATUS_RX_DP_SHIFT);
    *dn = ((pol & PMU_USB_POL_RX_DM) >> PMU_USB_POL_RX_DM_SHIFT) ^
        ((val & PMU_USB_STATUS_RX_DM) >> PMU_USB_STATUS_RX_DM_SHIFT);
}

void pmu_crash_config(void)
{
    hal_analogif_reg_write(0x6d,0x1701);
    *(volatile unsigned int *)(0x40000048) = 0x03ffffff;
    *(volatile unsigned int *)(0x4001f008) = 0xffffffff;
    *(volatile unsigned int *)(0x4001f020) = 0x00000000;
}

void pmu_charger_init(void)
{
    unsigned short readval_cfg;

    pmu_read(PMU_REG_CHARGER_CFG, &readval_cfg);
    readval_cfg &= ~0x1FFF;
    pmu_write(PMU_REG_CHARGER_CFG ,readval_cfg);
    hal_sys_timer_delay(MS_TO_TICKS(1));
    readval_cfg = 0|(1<<8)|(1<<9)|(1<<10)|(1<<11)|(1<<12);
    pmu_write(PMU_REG_CHARGER_CFG ,readval_cfg);
    hal_sys_timer_delay(MS_TO_TICKS(1));
}

void pmu_charger_plugin_config(void)
{
    pmu_module_set_volt(PMU_IO,0x14,0x13);
}

void pmu_charger_plugout_config(void)
{
    pmu_module_set_volt(PMU_IO,PMU_IO_2_7V,PMU_IO_2_6V);
}

enum PMU_CHARGER_STATUS_T pmu_charger_get_status(void)
{
    unsigned short readval;

    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
    {
        enum PMU_CHARGER_STATUS_T status = PMU_CHARGER_UNKNOWN;
        pmu_read(0x65, &readval);
            if (readval&(1<<10))
            status = PMU_CHARGER_PLUGIN;
        else
            status = PMU_CHARGER_PLUGOUT;

        TRACE("%s 0x65=0x%04x val:%d status:%d", __func__, readval, readval&(1<<10)?1:0,status);

        return status;
    }
    else
    {
        pmu_read(PMU_REG_CHARGER_CFG, &readval);
        readval &= ~(1<<8);
        pmu_write(PMU_REG_CHARGER_CFG ,readval);
        hal_sys_timer_delay(2);
        readval |= (1<<8);
        pmu_write(PMU_REG_CHARGER_CFG ,readval);
        hal_sys_timer_delay(2);
        pmu_read(PMU_REG_CHARGER_STATUS, &readval);
        pmu_write(PMU_REG_CHARGER_STATUS ,readval);

        if (readval & ((1<<0) | (1<<2))) {
            return PMU_CHARGER_PLUGIN;
        }

        return PMU_CHARGER_PLUGOUT;
    }
}

enum PMU_CHARGER_STATUS_T pmu_charger_detect(void)
{
    unsigned short readval;
    enum PMU_CHARGER_STATUS_T status = PMU_CHARGER_UNKNOWN;
    uint32_t lock = int_lock();

    pmu_read(PMU_REG_CHARGER_STATUS, &readval);
    pmu_write(PMU_REG_CHARGER_STATUS ,readval);
    TRACE("%s 0x6c=0x%02x", __func__, readval);

    if (!(readval&0x0f)){
        TRACE("%s SKIP", __func__);
    } else if ((readval & (1<<2)) && (readval & (1<<3))) {
        TRACE("%s DITHERING", __func__);
        pmu_read(PMU_REG_CHARGER_CFG, &readval);
        readval &= ~((1<<8)|(1<<11)|(1<<12));
        pmu_write(PMU_REG_CHARGER_CFG ,readval);
        hal_sys_timer_delay(2);
        readval |= (1<<8);
        pmu_write(PMU_REG_CHARGER_CFG ,readval);
        hal_sys_timer_delay(2);

        pmu_read(PMU_REG_CHARGER_STATUS, &readval);
        pmu_write(PMU_REG_CHARGER_STATUS, readval);
        if (readval & (1<<0)){
            status = PMU_CHARGER_PLUGIN;
            TRACE("%s PLUGIN 1", __func__);
        } else {
            status = PMU_CHARGER_PLUGOUT;
            TRACE("%s PLUGOUT 1", __func__);
        }
        readval &= ~0x1FFF;
        readval |= 0|(1<<8)|(1<<9)|(1<<10)|(1<<11)|(1<<12);
        pmu_write(PMU_REG_CHARGER_CFG ,readval);
    } else {
        TRACE("%s NORMAL", __func__);
        if (readval & (1<<2)){
            status = PMU_CHARGER_PLUGIN;
            TRACE("%s PLUGIN 2", __func__);
        }
        if (readval & (1<<3)){
            status = PMU_CHARGER_PLUGOUT;
            TRACE("%s PLUGOUT 2", __func__);
        }
    }

    int_unlock(lock);

    return status;
}

