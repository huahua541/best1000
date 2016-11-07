#include "cmsis_os.h"
#include "plat_types.h"
#include "string.h"
#include "hal_intersys.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "bt_drv.h"

extern void btdrv_hci_reset(void);
extern void btdrv_SendData(uint8_t *buff,uint8_t len);
extern void btdrv_enable_nonsig_tx(uint8_t index);
extern void btdrv_enable_nonsig_rx(uint8_t index);

struct dbg_nonsig_tester_result_tag
{
    uint16_t    pkt_counters;   
    uint16_t    head_errors;
    uint16_t    payload_errors;
    int16_t    avg_estsw;
    int16_t    avg_esttpl;
    uint32_t    payload_bit_errors;
};

struct bt_drv_capval_calc_t
{
    int16_t    estsw_a;
    int16_t    estsw_b;
    uint8_t    cdac_a;
    uint8_t    cdac_b;
};

static struct dbg_nonsig_tester_result_tag nonsig_tester_result;
static osThreadId calib_thread_tid = NULL;
static bool calib_running = false;
static struct bt_drv_capval_calc_t capval_calc ={
                                                .estsw_a = 0,
                                                .estsw_b = 0,
                                                .cdac_a = 0,
                                                .cdac_b = 0,
                                                };

#define bt_drv_calib_capval_calc_reset() do{ \
                                    memset(&capval_calc, 0, sizeof(capval_calc)); \
                                 }while(0)

#define BT_DRV_MUTUX_WAIT(evt) do{ \
                                if (calib_thread_tid){ \
                                    osSignalClear(calib_thread_tid, 0x4); \
                                    evt = osSignalWait(0x4, 8000); \
                                }\
                            }while(0)
                               
#define BT_DRV_MUTUX_SET() do{ \
                            if (calib_thread_tid){ \
                                osSignalSet(calib_thread_tid, 0x4); \
                            } \
                           }while(0)

#define BT_DRV_CALIB_BTANA_CAPVAL_MIN (10)
#define BT_DRV_CALIB_BTANA_CAPVAL_MAX (255-10)
#define BT_DRV_CALIB_BTANA_CAPVAL_STEP_HZ (200)

#define BT_DRV_CALIB_RETRY_CNT (5)
#define BT_DRV_CALIB_MDM_FREQ_REFERENCE (-28)
#define BT_DRV_CALIB_MDM_FREQ_STEP_HZ (300)
#define BT_DRV_CALIB_MDM_BIT_TO_FREQ(n, step) ((n)*(step))
#define BT_DRV_CALIB_MDM_FREQ_TO_BIT(n, step) ((uint32_t)n/step)

void bt_drv_start_calib(void)
{
    if (!calib_running){
        btdrv_enable_nonsig_rx(0);
        calib_running =  true;
    }
}
void bt_drv_stop_calib(void)
{
    if (calib_running){
        btdrv_hci_reset();
        calib_running =  false;
    }
}

void bt_drv_check_calib(void)
{
    btdrv_enable_nonsig_tx(0);
}

static bool bt_drv_calib_capval_calc_run(uint16_t *capval_step_hz, int16_t estsw, uint8_t cdac)
{
    bool nRet = false;
    int16_t estsw_diff;
    int16_t cdac_diff;

    if (!capval_calc.estsw_a &&
        !capval_calc.estsw_b){
        capval_calc.estsw_a = estsw;
    }else if (capval_calc.estsw_a &&
              !capval_calc.estsw_b){
        capval_calc.estsw_b = estsw;
    }

    if (!capval_calc.cdac_a &&
        !capval_calc.cdac_b){
        capval_calc.cdac_a = cdac;
    }else if (capval_calc.cdac_a &&
              !capval_calc.cdac_b){
        capval_calc.cdac_b = cdac;
    }

    if (capval_calc.estsw_a &&
        capval_calc.estsw_b &&
        capval_calc.cdac_a &&
        capval_calc.cdac_b){

        estsw_diff = ABS(capval_calc.estsw_b - capval_calc.estsw_a);
        cdac_diff = ABS((int16_t)(capval_calc.cdac_b) - (int16_t)(capval_calc.cdac_a));
        *capval_step_hz = estsw_diff*BT_DRV_CALIB_MDM_FREQ_STEP_HZ/cdac_diff;
        DRV_PRINT("%d/%d %d/%d estsw_diff:%d cdac_diff:%d capval_step_hz:%d", 
                                                                    capval_calc.estsw_a,
                                                                    capval_calc.estsw_b,
                                                                    capval_calc.cdac_a,
                                                                    capval_calc.cdac_b,
                                                                    estsw_diff,
                                                                    cdac_diff,
                                                                    *capval_step_hz);
        nRet = true;
    }
    return nRet;
}

void bt_drv_calib_rxonly_porc(void)
{
    osEvent evt;

	 /*[set tmx] */
    btdrv_write_rf_reg(0x11,0x9185); /* turn off tmx */

    while(1){
        bt_drv_start_calib();
        BT_DRV_MUTUX_WAIT(evt);
        bt_drv_stop_calib();
        if(evt.status != osEventSignal){
            break;
        }
        osDelay(10);
        DRV_PRINT("result cnt:%d head:%d payload:%d estsw:%d esttpl:%d bit:%d est:%d", nonsig_tester_result.pkt_counters,
                                                                                nonsig_tester_result.head_errors,
                                                                                nonsig_tester_result.payload_errors,
                                                                                nonsig_tester_result.avg_estsw,
                                                                                nonsig_tester_result.avg_esttpl,
                                                                                nonsig_tester_result.payload_bit_errors,
                                                                                nonsig_tester_result.avg_estsw+nonsig_tester_result.avg_esttpl);
        


    };
}

int bt_drv_calib_result_porc(uint32_t *capval)
{
    osEvent evt;

    uint16_t read_val = 0;
    uint8_t cdac = 0;
    uint8_t cnt = 0;
    int diff = 0;
    int est = 0;
    uint8_t next_step = 0;
    uint16_t capval_step_hz = BT_DRV_CALIB_BTANA_CAPVAL_STEP_HZ;
    int nRet = -1;

    /*[set default] */
    btdrv_read_rf_reg(0xc2, &read_val);
    cdac = *capval;            
    read_val = (read_val & 0x00ff)|((uint16_t)cdac<<8);
    btdrv_write_rf_reg(0xc2,read_val);

    /*[set tmx] */
    btdrv_write_rf_reg(0x11,0x9185); /* turn off tmx */

    bt_drv_calib_capval_calc_reset();

    do{
        bt_drv_start_calib();
        BT_DRV_MUTUX_WAIT(evt);
        bt_drv_stop_calib();
        if(evt.status != osEventSignal){
            nRet = -1;
            DRV_PRINT("evt:%d", evt.status);
            break;
        }
        osDelay(10);
        DRV_PRINT("result cnt:%d head:%d payload:%d estsw:%d esttpl:%d bit:%d est:%d",  nonsig_tester_result.pkt_counters,
                                                                                        nonsig_tester_result.head_errors,
                                                                                        nonsig_tester_result.payload_errors,
                                                                                        nonsig_tester_result.avg_estsw,
                                                                                        nonsig_tester_result.avg_esttpl,
                                                                                        nonsig_tester_result.payload_bit_errors,
                                                                                        nonsig_tester_result.avg_estsw+nonsig_tester_result.avg_esttpl);
        

        btdrv_read_rf_reg(0xc2, &read_val);
        cdac = (read_val & 0xff00)>> 8;            

        est = nonsig_tester_result.avg_estsw + nonsig_tester_result.avg_esttpl;
        diff = est-BT_DRV_CALIB_MDM_FREQ_REFERENCE;

        if (bt_drv_calib_capval_calc_run(&capval_step_hz, est, cdac)){
            bt_drv_calib_capval_calc_reset();
        }

        if ((BT_DRV_CALIB_MDM_BIT_TO_FREQ(ABS(diff), BT_DRV_CALIB_MDM_FREQ_STEP_HZ)< 3000) && 
            diff>0){
            nRet = 0;
            break;
        }else{
            next_step = BT_DRV_CALIB_MDM_BIT_TO_FREQ(ABS(diff), BT_DRV_CALIB_MDM_FREQ_STEP_HZ)/capval_step_hz;
            if (next_step == 0){
                next_step = 2;
            }
			if (next_step>200){
				next_step = 200;
			}
        }
        DRV_PRINT("diff:%d read_val:%x cdac:%x next_step:%d", diff,read_val, cdac, next_step);
        if (est == BT_DRV_CALIB_MDM_FREQ_REFERENCE){
            nRet = 0;
            break;
        }else if (est > BT_DRV_CALIB_MDM_FREQ_REFERENCE){
            if (cdac < (BT_DRV_CALIB_BTANA_CAPVAL_MIN+next_step)){
                cdac = BT_DRV_CALIB_BTANA_CAPVAL_MIN;
            }else{
                cdac -= next_step;
            }
            read_val = (read_val & 0x00ff)|((uint16_t)cdac<<8);
            
            DRV_PRINT("-----:%x cdac:%x", read_val, cdac);
        }else if (est < BT_DRV_CALIB_MDM_FREQ_REFERENCE){
            if (cdac>(BT_DRV_CALIB_BTANA_CAPVAL_MAX-next_step)){
                cdac = BT_DRV_CALIB_BTANA_CAPVAL_MAX;
            }else{
                cdac += next_step;
            }
            read_val = (read_val & 0x00ff)|((uint16_t)cdac<<8);
            
            DRV_PRINT("+++++:%x cdac:%x", read_val, cdac);
        }
        
        btdrv_write_rf_reg(0xc2,read_val);
        osDelay(100);
    }while(cnt++<BT_DRV_CALIB_RETRY_CNT);
    
    *capval = cdac;
    DRV_PRINT("capval:0x%x cnt:%d nRet:%d", *capval, cnt, nRet);
    return nRet;
}

static unsigned int bt_drv_calib_rx(const unsigned char *data, unsigned int len)
{
    const unsigned char nonsig_test_report[] = {0x04, 0x0e, 0x12};
    struct dbg_nonsig_tester_result_tag *pResult = NULL;

    hal_intersys_stop_recv(HAL_INTERSYS_ID_0);

//    DRV_PRINT("%s len:%d", __func__, len);
//    DRV_DUMP("%02x ", data, len>7?7:len);
    if (0 == memcmp(data, nonsig_test_report, sizeof(nonsig_test_report))){
//        DRV_PRINT("dbg_nonsig_tester_result !!!!!");
        pResult = (struct dbg_nonsig_tester_result_tag *)(data + 7);
        memcpy(&nonsig_tester_result, pResult, sizeof(nonsig_tester_result));
        BT_DRV_MUTUX_SET();
    }
    hal_intersys_start_recv(HAL_INTERSYS_ID_0);

    return len;
}

void bt_drv_calib_open(void)
{
    int ret = 0;
    DRV_PRINT("%s", __func__);

    if (calib_thread_tid == NULL){
        calib_thread_tid = osThreadGetId();
    }
    
    ret = hal_intersys_open(HAL_INTERSYS_ID_0, HAL_INTERSYS_MSG_HCI, bt_drv_calib_rx, NULL);

    if (ret) {
        DRV_PRINT("Failed to open intersys");
        return;
    }
    hal_intersys_start_recv(HAL_INTERSYS_ID_0);
}

void bt_drv_calib_close(void)
{
    btdrv_hci_reset();
    osDelay(200);
    hal_intersys_close(HAL_INTERSYS_ID_0,HAL_INTERSYS_MSG_HCI);
}


