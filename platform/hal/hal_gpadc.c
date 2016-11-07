#include "stddef.h"
#include "cmsis_nvic.h"
#include "hal_gpadc.h"
#include "hal_trace.h"
#include "hal_analogif.h"
#include "pmu.h"

#define HAL_GPADC_TRACE(s,...) //hal_trace_printf(s, ##__VA_ARGS__)

#define gpadc_reg_read(reg,val)  hal_analogif_reg_read(reg,val)
#define gpadc_reg_write(reg,val) hal_analogif_reg_write(reg,val)

// Battery voltage = gpadc voltage * 4
// adc rate 0~2v(10bit)
// Battery_voltage:Adc_rate = 4:1
#define HAL_GPADC_MVOLT_A   800
#define HAL_GPADC_MVOLT_B   1050
#define HAL_GPADC_CALIB_DEFAULT_A   428
#define HAL_GPADC_CALIB_DEFAULT_B   565

static int32_t g_adcSlope = 0;
static int32_t g_adcIntcpt = 0;
static bool  g_adcCalibrated = false;
static HAL_GPADC_EVENT_CB_T gpadc_event_cb[HAL_GPADC_CHAN_QTY];

static int hal_gpadc_adc2volt_calib(void)
{
    int32_t y1, y2, x1, x2;
    unsigned short efuse;

    if (!g_adcCalibrated)
    {
        y1 = HAL_GPADC_MVOLT_A*1000;
        y2 = HAL_GPADC_MVOLT_B*1000;

        efuse = 0;
        pmu_get_efuse(PMU_EFUSE_PAGE_BATTER_LV, &efuse);

        x1 = efuse > 0 ? efuse : HAL_GPADC_CALIB_DEFAULT_A;

        efuse = 0;
        pmu_get_efuse(PMU_EFUSE_PAGE_BATTER_HV, &efuse);
        x2 = efuse > 0 ? efuse : HAL_GPADC_CALIB_DEFAULT_B;

        g_adcSlope = (y2-y1)/(x2-x1);
        g_adcIntcpt = ((y1*x2)-(x1*y2))/((x2-x1)*1000);
        g_adcCalibrated = true;

        TRACE("%s LV=%d, HV=%d, Slope:%d Intcpt:%d",__func__, x1, x2, g_adcSlope, g_adcIntcpt);
    }

    return 0;
}

static HAL_GPADC_MV_T hal_gpadc_adc2volt(uint16_t gpadcVal)
{
    int32_t voltage;

    hal_gpadc_adc2volt_calib();
    if (gpadcVal == HAL_GPADC_BAD_VALUE)
    {
        // Bad values from the GPADC are still Bad Values
        // for the voltage-speaking user.
        return HAL_GPADC_BAD_VALUE;
    }
    else
    {
        voltage = (((g_adcSlope*gpadcVal)/1000)+(g_adcIntcpt));

        return voltage<0 ? 0 : voltage;
    }
}

static void hal_gpadc_irqhandler(void)
{
	uint8_t i;
	unsigned short read_val = 0;
	unsigned short reg_6a = 0;
	HAL_GPADC_MV_T adc_val = 0;
	gpadc_reg_read(0x6a, &read_val);
	gpadc_reg_write(0x6a,read_val);
	reg_6a = read_val;

	if (read_val & ((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7))){
		for (i=0; i<7; i++){
			if (read_val & (1<<i))
				break;
		}
        while((read_val & 0x100)==0){
            gpadc_reg_read(0x6a, &read_val);
        }
		switch (i) {
			case HAL_GPADC_CHAN_BATTERY:
				gpadc_reg_read(0x45,&read_val);
				read_val &= ~(1<<0);
				gpadc_reg_write(0x45, read_val);
			case HAL_GPADC_CHAN_0:
			case HAL_GPADC_CHAN_2:
			case HAL_GPADC_CHAN_3:
			case HAL_GPADC_CHAN_4:
			case HAL_GPADC_CHAN_5:
			case HAL_GPADC_CHAN_6:
				//gpadc chan1
				gpadc_reg_read(0x79, &read_val);
				read_val &= 0x03ff;
				adc_val = hal_gpadc_adc2volt(read_val);
				if (gpadc_event_cb[i])
					gpadc_event_cb[i]((void *)&adc_val);

				gpadc_reg_read(0x60, &read_val);
				if (!(read_val & (1<<12))){
					//mask
					gpadc_reg_read(0x67,&read_val);
					read_val &= ~(1<<i);
					gpadc_reg_write(0x67, read_val);
					//enable
					gpadc_reg_read(0x68,&read_val);
					read_val &= ~(1<<i);
					gpadc_reg_write(0x68, read_val);

					// Enable gpadc module and channel
					gpadc_reg_read(0x65,&read_val);
					read_val &= ~(1<<i);
					gpadc_reg_write(0x65, read_val);
				}
				break;
			default:
				break;
		}
	}
	//need disable  adc module?
	gpadc_reg_read(0x65,&read_val);
	if (!(read_val & ((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)))){
		read_val &= ~(1<<8);
		gpadc_reg_write(0x65, read_val);
	}

	if (reg_6a & ((1<<7)|(1<<9)|(1<<10)|(1<<11)|(1<<12))){
		if (gpadc_event_cb[HAL_GPADC_CHAN_ADCKEY])
			gpadc_event_cb[HAL_GPADC_CHAN_ADCKEY](&reg_6a);
	}

	if(read_val & (1<<13)){
		//rtc int0
	}else if(read_val & (1<<14)){
		//rtc int1
	}else if(read_val & (1<<15)){
		//usb_int
	}
}

int hal_gpadc_open(enum HAL_GPADC_CHAN_T channel, enum HAL_GPADC_ATP_T atp, HAL_GPADC_EVENT_CB_T cb)
{
    unsigned short val;

	if (channel >= HAL_GPADC_CHAN_QTY)
		return -1;

	gpadc_event_cb[channel] = cb;

	switch (channel) {
		case HAL_GPADC_CHAN_ADCKEY:
			break;
		case HAL_GPADC_CHAN_BATTERY:
			// Enable vbat measurement clock
			// vbat div enable
			gpadc_reg_read(0x45,&val);
			val |= (1<<0);
			gpadc_reg_write(0x45, val);
		case HAL_GPADC_CHAN_0:
		case HAL_GPADC_CHAN_2:
		case HAL_GPADC_CHAN_3:
		case HAL_GPADC_CHAN_4:
		case HAL_GPADC_CHAN_5:
		case HAL_GPADC_CHAN_6:
			//mask
			gpadc_reg_read(0x67,&val);
			val |= 1<<channel;
			gpadc_reg_write(0x67, val);
			//enable
			gpadc_reg_read(0x68,&val);
			val |= 1<<channel;
			gpadc_reg_write(0x68, val);

			// Enable gpadc module and channel
			gpadc_reg_read(0x65,&val);
			val |= (1<<8)|(1<<channel);
			gpadc_reg_write(0x65, val);
			break;
		default:
			break;
	}

	NVIC_SetVector(GPADC_IRQn, (uint32_t)hal_gpadc_irqhandler);
	NVIC_SetPriority(GPADC_IRQn, IRQ_PRIORITY_NORMAL);
	NVIC_EnableIRQ(GPADC_IRQn);

	return 0;
}

int hal_gpadc_close(enum HAL_GPADC_CHAN_T channel)
{
    unsigned short val;

	switch (channel) {
		case HAL_GPADC_CHAN_ADCKEY:
			break;
		case HAL_GPADC_CHAN_BATTERY:
			// disable vbat measurement clock
			// vbat div disable
			gpadc_reg_read(0x45,&val);
			val &= ~(1<<0);
			gpadc_reg_write(0x45, val);
		case HAL_GPADC_CHAN_0:
		case HAL_GPADC_CHAN_2:
		case HAL_GPADC_CHAN_3:
		case HAL_GPADC_CHAN_4:
		case HAL_GPADC_CHAN_5:
		case HAL_GPADC_CHAN_6:
			//mask
			gpadc_reg_read(0x67,&val);
			val &= ~(1<<channel);
			gpadc_reg_write(0x67, val);
			//enable
			gpadc_reg_read(0x68,&val);
			val &= ~(1<<channel);
			gpadc_reg_write(0x68, val);

			// disable gpadc module and channel
			gpadc_reg_read(0x65,&val);
			val &= ~(1<<channel);
			gpadc_reg_write(0x65, val);
			break;
		default:
			break;
	}

	gpadc_reg_read(0x65,&val);
	if (!(val & ((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)))){
		val &= ~(1<<8);
		gpadc_reg_write(0x65, val);
	}

	if (!(val & ((1<<8)|(1<<9)))){
		NVIC_SetVector(GPADC_IRQn, (uint32_t)NULL);
		NVIC_DisableIRQ(GPADC_IRQn);
	}

	return 0;
}

void hal_gpadc_sleep(void)
{
    unsigned short val;

    gpadc_reg_read(0x65,&val);
    if (val & (1<<8)) {
        val &= ~(1<<8);
        gpadc_reg_write(0x65, val);
    }
}

void hal_gpadc_wakeup(void)
{
    unsigned short val;

    gpadc_reg_read(0x65,&val);
    if (val & ((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6))) {
        val |= (1<<8);
        gpadc_reg_write(0x65, val);
    }
}

