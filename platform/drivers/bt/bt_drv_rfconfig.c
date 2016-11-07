#include "plat_types.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "bt_drv.h"
#include "cmsis_os.h"
#include "hal_cmu.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "bt_drv_internal.h"

#define AUTO_CAL                                1
//#define BT_LOW_POWER_MODE                       1
//#define BT_HIGH_PERFORMANCE_MODE                2
//#define BT_RFMODE                               (BT_LOW_POWER_MODE)

#define DEFAULT_XTAL_FCAP                       0x8000

#define XTAL_FCAP_NORMAL_SHIFT                  8
#define XTAL_FCAP_NORMAL_MASK                   (0xFF << XTAL_FCAP_NORMAL_SHIFT)
#define XTAL_FCAP_NORMAL(n)                     BITFIELD_VAL(XTAL_FCAP_NORMAL, n)

/* ljh add for sync modify rf c2 value*/
#define SPI_TRIG_NEG_BUF_ADDR                   0xD0217208
#define SPI_TRIG_CTRL_ADDR                      0xD02201C0
#define SPI_TRIG_NEG_ON                         (1 << 8)

#ifdef BTDRV_XTAL_FCAP_DIRECT_UPDATE
#define BIT_OFFSET_VALUE_REG                    0xD02101C8
#else
#define BIT_OFFSET_CMD_REG                      0xD0217210
#define BIT_OFFSET_VALUE_REG                    0xD0217214
#endif
#define BIT_OFFSET_MODE_REG                     0xD02200F0

#define BIT_OFFSET_MODE_AUTO_TRACK              (1 << 0)

enum BIT_OFFSET_CMD_TYPE_T {
    BIT_OFFSET_CMD_STOP = 0,
    BIT_OFFSET_CMD_START,
    BIT_OFFSET_CMD_ACK,
};

static uint16_t xtal_fcap = DEFAULT_XTAL_FCAP;

#if (BT_RFMODE == BT_LOW_POWER_MODE)
const uint16_t rf_init_tbl[][3] =
{
#ifndef _BEST1000_QUAL_DCDC_
	/*[pa init]*/
	{0x50,0x0016,0},
#endif

#if 0
	{0xe0 ,0x472B,0}, /* 51dDB*/
	{0xe1 ,0xC713,0}, /* 42dDB*/
	{0xe2 ,0xC702,0}, /* 29dDB*/
	{0xe3 ,0xC7c2,0}, /* 15dDB*/
	{0xe4 ,0xC7c2,0}, /* 15dDB*/
#else
	{0xe0 ,0x472b,0}, /* 57dDB*/
	{0xe1 ,0xC704,0}, /* 57dDB*/
	{0xe2 ,0xC104,0}, /* 47dDB*/
	{0xe3 ,0xC003,0}, /* 36dDB*/
	{0xe4 ,0xC043,0}, /* 26dDB*/
	{0xe5 ,0xC0c3,0}, /* 16dDB*/
	{0xe6 ,0xC0c2,0}, /* 10dDB*/

#endif

/*set tx gain table */
//	{0xe8,0x3a00,0},
#if 0
	{0xe8,0x3a04,0},
	{0xe9,0x3a01,0},
	{0xea,0x3a05,0},
	{0xeb,0x3a02,0},
	{0xec,0x3a06,0},
	{0xed,0x3a07,0},
	{0xee,0x3a23,0},
	{0xef,0x3a0e,0},
	      3a0f
	      3a2f
	      3a1f
	      3a3f
	      3a67
	      3a78
	      3a5f
	      3ae7
#elif 1
	{0xe8,0x3A00,0},
	{0xe9,0x3A04,0},
	{0xea,0x3A05,0},
	{0xeb,0x3A05,0},
	{0xec,0x3A05,0},
	{0xed,0x3A05,0},
	{0xee,0x3A05,0},
	{0xef,0x3A05,0},
#else
	{0xe8,0x3a04,0},
	{0xe9,0x3a02,0},
	{0xea,0x3a23,0},
	{0xeb,0x3a23,0},
	{0xec,0x3a23,0},
	{0xed,0x3a23,0},
	{0xee,0x3a23,0},
	{0xef,0x3a23,0},
#endif

	 /*[xtal cal] */
	{0xc2,DEFAULT_XTAL_FCAP,0},//////0x0000--->5400
	///xtal drv and bias
       //{0xc4,0x4994,0},
       //xtal wifi buf close
       {0x1d,0x28a4,0},
       //xtal extra buf close
   //     {0xc7,0xd008,0},

	 /*[vco init] */
	{0x18,0x0070,0}, /*0x077f */
    {0x19,0x3C60,0}, /*0x3C60 */

	{0x1a,0x0000,0}, /*0xc010 */
	{0x1b,0x0888,0},
	{0x1c,0x0446,0},

	 /*[rfpll_cal] */
	{0x21,0x3979,0}, /* ref sel 52MHz */
	{0x22,0x7A22,0}, /* doubler setting */
	{0x23,0x0380,0}, /* doubler setting */
	{0x2b,0x32a0,0}, /* sdm */
	{0x2a,0x02d1,0}, /* cal ini */
	{0x28,0x038f,0}, /* rx 890k low if */

	 /*[set tmx] */
	{0x11,0x9145,0}, /* turn on tmx */
	{0x12,0x3188,0}, /* weak tmx buff */

	 /*[pa init] */
	{0x15,0x1060,0},
//    {0x16,0x1ad4,0},
    {0x16,0x7fd4,0},
	{0x2b,0x30a0,0}, /*{0x36a0 */
	{0x0f,0x0a18,0},

	 /*[tmx cal] */
	{0x10,0x40b1,0},

#ifndef AUTO_CAL
	{0x10,0x4cb1,0},
	{0xc8,0x8080,0},
#endif

	 /*[RX RF setting] */
	 /*[lna init] */
	{0x02,0x0094,0},
#if 0
	{0x05,0x041c,0},
	{0x06,0x1188,0},
#else
	{0x05,0x041c,0},///modified by gibson for reduce rx current
	{0x06,0x1488,0},
#endif
	{0x0b,0x272b,0},

	 /*[rx_filter_initial] */
	{0x0a,0x0725,0}, /*072b*/
	{0x0d,0x3e88,0},

	{0x13,0xcC43,0},
    {0x2e,0x6aaa,0},

	 /*[set Adc] */
	{0x0c,0x2790,0},

};

#elif (BT_RFMODE == BT_HIGH_PERFORMANCE_MODE)

uint16_t rf_init_tbl[][3] =
{
#ifndef _BEST1000_QUAL_DCDC_

	/*[pa init]*/
	{0x50,0x0016,0},
#endif

#if 0
	{0xe0 ,0x472B,0}, /* 51dDB*/
	{0xe1 ,0xC70D,0}, /* 39dDB*/
	{0xe2 ,0xC702,0}, /* 29dDB*/
	{0xe3 ,0xC742,0}, /* 15dDB*/
	{0xe4 ,0xC742,0}, /* 15dDB*/
#else
	{0xe0 ,0x472B,0}, /* 51dDB*/
	{0xe1 ,0xC704,0}, /* 39dDB*/
	{0xe2 ,0xC303,0}, /* 29dDB*/
	{0xe3 ,0xC003,0}, /* 15dDB*/
	{0xe4 ,0xC043,0}, /* 15dDB*/
	{0xe5 ,0xC0c3,0}, /* 15dDB*/
//	{0xe6 ,0xC0c0,0}, /* 15dDB*/


#endif
	 /*set tx gain table */
//	{0xe8,0x3a00,0},
	{0xe8,0x3a04,0},
	{0xe9,0x3a01,0},
	{0xea,0x3a05,0},
	{0xeb,0x3a02,0},
	{0xec,0x3a06,0},
	{0xed,0x3a07,0},
	{0xee,0x3a23,0},
	{0xef,0x3a0e,0},




	 /*[xtal cal] */
	{0xc2,DEFAULT_XTAL_FCAP,0},

	 /*[vco init] */
	{0x18,0x007f,0}, /*0x077f */
	{0x19,0x3C60,0}, /*0x3C60 */

	{0x1a,0x0000,0}, /*0xc010 */
	{0x1b,0x0f88,0},
	{0x1c,0x0446,0},

	 /*[rfpll_cal] */
	{0x21,0x3979,0}, /* ref sel 52MHz */
	{0x22,0x7A22,0}, /* doubler setting */
	{0x23,0x0380,0}, /* doubler setting */
	{0x2b,0x32a0,0}, /* sdm */
	{0x2a,0x02d1,0}, /* cal ini */
	{0x28,0x038f,0}, /* rx 890k low if */

	 /*[set tmx] */
	{0x11,0x9145,0}, /* turn on tmx */

	{0x12,0x3188,0}, /* weak tmx buff */

	 /*[pa init] */
	{0x15,0x1060,0},
    {0x16,0x7ad4,0},
	{0x2b,0x30a0,0}, /*{0x36a0 */
	{0x0f,0x0a18,0},

	 /*[tmx cal] */
	{0x10,0x40b1,0},

#ifndef AUTO_CAL
	{0x10,0x4cb1,0},
	{0xc8,0x8080,0},
#endif

	 /*[RX RF setting] */
	 /*[lna init] */
	{0x02,0x0094,0},
	{0x05,0x049c,0},
	{0x06,0x1488,0},
	{0x0b,0x272b,0},

	 /*[rx_filter_initial] */

	{0x0a,0x0725,0}, /*072b*/
	{0x0d,0x3e88,0},


	{0x13,0xcC43,0},
	{0x2e,0x6aaa,0},

	 /*[set Adc] */
	{0x0c,0x2790,0},
};
#endif

void btdrv_rf_init_xtal_fcap(uint32_t fcap)
{
    xtal_fcap = SET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL, fcap);
    btdrv_write_rf_reg(0xC2, xtal_fcap);
}

uint32_t btdrv_rf_get_xtal_fcap(void)
{
    return GET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL);
}

void btdrv_rf_set_xtal_fcap(uint32_t fcap)
{
    xtal_fcap = SET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL, fcap);
#ifdef BTDRV_XTAL_FCAP_DIRECT_UPDATE
    btdrv_write_rf_reg(0xC2, xtal_fcap);
#else
    // Set the 0xC2 value and the flag in BT controller
    uint16_t value = ((xtal_fcap & 0xFF00) >> 8) | ((xtal_fcap & 0xFF) << 8);
    BTDIGITAL_REG(SPI_TRIG_NEG_BUF_ADDR) = (BTDIGITAL_REG(SPI_TRIG_NEG_BUF_ADDR) & 0xFF0000FF) |
        (value << 8);
    BTDIGITAL_REG(SPI_TRIG_CTRL_ADDR) |= SPI_TRIG_NEG_ON;
#endif
}

int btdrv_rf_xtal_fcap_busy(void)
{
#ifdef BTDRV_XTAL_FCAP_DIRECT_UPDATE
    return 0;
#else
    if (BTDIGITAL_REG(SPI_TRIG_CTRL_ADDR) & SPI_TRIG_NEG_ON) {
        BTDIGITAL_REG(SPI_TRIG_CTRL_ADDR) &= ~SPI_TRIG_NEG_ON;
#ifdef DEBUG_FCAP_SPI_UPDATE
        uint16_t val;
        btdrv_read_rf_reg(0xC2, &val);
        TRACE("*** NEW C2 reg: 0x%04x fcap=%u\n\n", val, val >> 8);
#endif
        return 1;
    }
    if (BTDIGITAL_REG(BIT_OFFSET_CMD_REG) != BIT_OFFSET_CMD_ACK) {
        return 1;
    }
    return 0;
#endif
}

void btdrv_rf_bit_offset_track_enable(bool enable)
{
    // BT controller seems to set this register too
    if (enable) {
        BTDIGITAL_REG(BIT_OFFSET_MODE_REG) &= ~BIT_OFFSET_MODE_AUTO_TRACK;
#ifndef BTDRV_XTAL_FCAP_DIRECT_UPDATE
        BTDIGITAL_REG(BIT_OFFSET_CMD_REG) = BIT_OFFSET_CMD_START;
#endif
    } else {
        BTDIGITAL_REG(BIT_OFFSET_MODE_REG) |= BIT_OFFSET_MODE_AUTO_TRACK;
#ifndef BTDRV_XTAL_FCAP_DIRECT_UPDATE
        BTDIGITAL_REG(BIT_OFFSET_CMD_REG) = BIT_OFFSET_CMD_STOP;
#endif
    }
}

uint32_t btdrv_rf_bit_offset_get(void)
{
    return BTDIGITAL_REG(BIT_OFFSET_VALUE_REG);
}

uint8_t btdrv_rf_rxdc_cal(void)
{
	uint16_t read_val = 0;
	uint16_t reg0b_bak = 0;
	uint8_t i=0;
	uint8_t cnt = 0;

	TRACE("%s \n", __func__);

	BTDIGITAL_REG(0xd0340020) |= 0x5; /* rxon always on */

	btdrv_read_rf_reg(0xf6, &read_val);
	read_val &= 0xfe0f;
	read_val |= 25 <<4;
	btdrv_write_rf_reg(0xf6,read_val); /* cal clk 0.5mhz */
	btdrv_write_rf_reg(0x08,0x1f1f);   /* cal wait time */

	btdrv_read_rf_reg(0x02, &read_val);
	read_val |= 1<<11; /*lna_att_dr = 1*/
	btdrv_write_rf_reg(0x02,read_val); /* lna_att_dr =1 */

	btdrv_read_rf_reg(0x0b, &read_val);
	reg0b_bak = read_val;
	read_val |= 3<<6; /*lna_att = 2'b11*/
	btdrv_write_rf_reg(0x0b,read_val); /* lna_att =2'b11 */

	for(i=0; i<16; i++)
	{
		btdrv_write_rf_reg(0x09,(1<<4)|i);   /* cal gain set */


		btdrv_read_rf_reg(0x07, &read_val);
		read_val &= 0xfffe;

		btdrv_write_rf_reg(0x07,read_val);   /* cal dig rst */
		read_val &= 0xdfff;        /* cal2 disable */
		read_val |= (1<<14)|(1<<6)|(1);
		btdrv_write_rf_reg(0x07,read_val);   /* cal1 dig start */

		/* det 1 is cal1 done */
		cnt = 0;
		do{
			btdrv_delay(1);
			btdrv_read_rf_reg(0xc0, &read_val);
			if (read_val &(1<<5))
				break;
		}while ((++cnt)<100);
		DRV_PRINT("%s cnt1:%d\n", __func__, cnt);
		if (cnt>=100) return 0;

		btdrv_read_rf_reg(0xd0, &read_val);  /* read cal1 value */

		if ( i < 8 ){
			btdrv_write_rf_reg((0xd0+i), read_val);  /* write cal1 value */
		}
		else{
			btdrv_write_rf_reg((0x1d0+i-8), read_val);;  /* write cal1 value */
		}

		btdrv_read_rf_reg(0x07, &read_val);
		read_val &= 0xbfff;        /* cal1 disable */
		read_val |= (1<<13)|(1<<6)|1;
		btdrv_write_rf_reg(0x07, read_val);   /* cal2 dig start */


		 /* det 1 is cal2 done */
		cnt = 0;
		do{
			btdrv_delay(1);
			btdrv_read_rf_reg(0xc0, &read_val);
			if (read_val&(1<<6))
				break;
		}while((++cnt)<100);
		DRV_PRINT("%s cnt2:%d\n", __func__, cnt);
		if (cnt>=100) return 0;

		btdrv_read_rf_reg(0xd8, &read_val);  /* read cal2 value */
		if ( i < 8 ){
			btdrv_write_rf_reg((0xd8+i), read_val);  /* write cal2 value */
		}else{
			btdrv_write_rf_reg((0x1d8+i-8), read_val);  /* write cal2 value */
		}
	}

	btdrv_write_rf_reg(0x0b,reg0b_bak);

	btdrv_read_rf_reg(0x07, &read_val);
	read_val &= 0xbfff;        /* cal1 disable */
	read_val &= 0xdfff;        /* cal2 disable */
	read_val &= 0xffbf;        /* calen disable */
	btdrv_write_rf_reg(0x07,read_val);   /* cal dig stop */

	btdrv_read_rf_reg(0x02, &read_val);
	read_val &= 0xf7ff;      /* lna_att_dr =0*/
	btdrv_write_rf_reg(0x02,read_val); /* lna_att_dr =0*/

	btdrv_write_rf_reg(0x09,0);   /* cal gain dis */

	BTDIGITAL_REG(0xd0340020)	&= 0xfffffff8; /* rxon off */

	return 1;
}

uint8_t btdrv_rf_txdc_cal(void)
{
	uint16_t read_val = 0;
	uint8_t i=0;
	uint8_t cnt = 0;

	TRACE("%s \n", __func__);

	BTDIGITAL_REG(0xd0340020) |= 0x6; /* txon always on */

	btdrv_read_rf_reg(0xf6, &read_val);
	read_val &= 0xfe0f ;
	read_val |= 25 <<4;
	btdrv_write_rf_reg(0xf6,read_val); /* cal clk 0.5mhz */
	btdrv_write_rf_reg(0x08,0x1f1f);   /* cal wait time */

	btdrv_read_rf_reg(0x10, &read_val);
	read_val &= 0x0ff0 ;
	read_val |= 4<<12 | 4;   /*cal wait time and dac mux en*/
	btdrv_write_rf_reg(0x10,read_val); /*cal wait time and dac mux en*/

	for(i=0; i<8; i++)
	{
		btdrv_write_rf_reg(0x09,(1<<12)|(i<8));   /* cal tx gain set */

		btdrv_read_rf_reg(0x11, &read_val);
		read_val &= 0xcfff;/*cal tx initial del clr */
		read_val |= 1<<12; /* cal tx initial del */
		read_val &= 0xbfff;/*cal tx en clr */
		read_val &= 0x7fff;/*cal tx rst enable */
		btdrv_write_rf_reg(0x11,read_val);   /* cal dig rst */
		read_val |= 1<<15; /* cal tx rst stop */
		btdrv_write_rf_reg(0x11,read_val);   /* cal dig rst */

		read_val |= 1<<14; /* cal tx start */
		btdrv_write_rf_reg(0x11,read_val);   /* cal dig start */

		/* det 1 is cal done */
		cnt = 0;
		do{
			btdrv_delay(1);
			btdrv_read_rf_reg(0xc0, &read_val);
			if (read_val & (1<<4))
				break;
		}while ((++cnt)<100);
		DRV_PRINT("%s cnt1:%d\n", __func__, cnt);
		if (cnt>=100) return 0;

		btdrv_read_rf_reg(0xc8, &read_val);  /* read cal value */

		btdrv_write_rf_reg((0xc8+i), read_val);  /* write cal value */
	}

	btdrv_read_rf_reg(0x11, &read_val);
	read_val &= 0xbfff;        /* cal disable */
	btdrv_write_rf_reg(0x11,read_val);   /* cal dig stop */

	btdrv_read_rf_reg(0x10, &read_val);
	read_val &= 0xfff0;      /* dac mux en = 0*/
	read_val |= 1;           /* dac mux en = 1*/
	btdrv_write_rf_reg(0x10,read_val); /* lna_att_dr =0*/

	btdrv_write_rf_reg(0x09,0);   /* cal gain dis */

	BTDIGITAL_REG(0xd0340020)	&= 0xfffffff8; /* txon off */

	return 1;
}


uint8_t btdrv_rf_init(void)
{
    for(uint8_t i=0;i< sizeof(rf_init_tbl)/sizeof(rf_init_tbl[0]);i++)
    {
        btdrv_write_rf_reg(rf_init_tbl[i][0],rf_init_tbl[i][1]);
        if(rf_init_tbl[i][2] !=0)
            btdrv_delay(rf_init_tbl[i][2]);//delay
    }

#ifdef AUTO_CAL
	btdrv_rf_rxdc_cal();
	btdrv_rf_txdc_cal();
#endif
    return 1;
}

