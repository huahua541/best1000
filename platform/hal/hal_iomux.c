#include "plat_types.h"
#include "reg_iomuxip.h"
#include "hal_iomuxip.h"
#include "hal_iomux.h"
#include "hal_uart.h"
#include "hal_trace.h"
#include "hal_timer.h"

/* i2c mode is specail : 0 means i2c, 1 means gpio */
#define I2C_IS_SPECIAL 1

#define HAL_IOMUX_YES 1
#define HAL_IOMUX_NO 0

#define HAL_IOMUX_MAP_NUM 6

struct pin_function_regbit_map {
    enum HAL_IOMUX_FUNCTION_T func;
    uint32_t reg_offset;
    uint32_t bit_mask;
};

/* see 192.168.1.3/dokuwiki/doku.php?id=projects:best1000#pinpad */
/* should put alt function in priority order (see rtl) */
const struct pin_function_regbit_map _pfr_map[HAL_IOMUX_PIN_NUM][HAL_IOMUX_MAP_NUM+1] = {
    /* 0 0 */ { 
                {HAL_IOMUX_FUNC_SDIO_CLK, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDIO_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 0 1 */ { 
                {HAL_IOMUX_FUNC_SDIO_CMD, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDIO_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 0 2 */ { 
                {HAL_IOMUX_FUNC_SDIO_DATA0, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDIO_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 0 3 */ {
                {HAL_IOMUX_FUNC_SDIO_DATA1, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDIO_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 0 4 */ {
                {HAL_IOMUX_FUNC_SDIO_DATA2, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDIO_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 0 5 */ {
                {HAL_IOMUX_FUNC_SDIO_DATA3, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDIO_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 0 6 */ {
                {HAL_IOMUX_FUNC_SDIO_INT_M, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDIO_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 0 7 */ {
                {HAL_IOMUX_FUNC_SDEMMC_RST_N, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_CLK_32K_IN, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_CLK32K_IN_MASK}, 
                {HAL_IOMUX_FUNC_SPI_CS, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPI2_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SMP0, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SMP_EN_MASK}, 
                {HAL_IOMUX_FUNC_RF_ANA_SWTX, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_RF_ANA_SW2_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },

    /* 1 0 */ {
                {HAL_IOMUX_FUNC_I2S_WS, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S1_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPDIF_DI, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPDIF1_DIN_EN_MASK}, 
                {HAL_IOMUX_FUNC_RF_ANA_SWTX, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_RF_ANA_SW1_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 1 1 */ {
                {HAL_IOMUX_FUNC_I2S_SDI, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S1_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPDIF_DO, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPDIF1_DOUT_EN_MASK}, 
                {HAL_IOMUX_FUNC_RF_ANA_SWRX, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_RF_ANA_SW1_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 1 2 */ {
                {HAL_IOMUX_FUNC_I2S_SDO, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S1_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_MIC_DI, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_MIC_DIG_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 1 3 */ {
                {HAL_IOMUX_FUNC_I2S_SCK, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S1_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_MIC_CLK, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_MIC_DIG_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 1 4 */ {
                {HAL_IOMUX_FUNC_SDMMC_CLK, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_I2S_WS, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S2_GPIO_EN_MASK}, 
                /* TODO agpio 0 */
                {HAL_IOMUX_FUNC_SMP1, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SMP_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 1 5 */ {
                {HAL_IOMUX_FUNC_SDMMC_CMD, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_I2S_SDI, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S2_GPIO_EN_MASK}, 
                /* TODO agpio 1 */
                {HAL_IOMUX_FUNC_SMP2, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SMP_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 1 6 */ {
                {HAL_IOMUX_FUNC_SDMMC_DATA0, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_I2S_SDO, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S2_GPIO_EN_MASK}, 
                /* TODO agpio 2 */
                {HAL_IOMUX_FUNC_SMP3, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SMP_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 1 7 */ {
                {HAL_IOMUX_FUNC_SDMMC_DATA1, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_I2S_SCK, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S2_GPIO_EN_MASK}, 
                /* TODO agpio 3 */
                {HAL_IOMUX_FUNC_SMP4, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SMP_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },

    /* 2 0 */ {
                {HAL_IOMUX_FUNC_SDMMC_DATA2, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_MIC_CLK, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_MIC2_DIG_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPI_CLK, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPI2_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SMP5, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SMP_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 2 1 */ {
                {HAL_IOMUX_FUNC_SDMMC_DATA3, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_MIC_DI, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_MIC2_DIG_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPI_DO, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPI2_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SMP6, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SMP_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 2 2 */ {
                {HAL_IOMUX_FUNC_SDMMC_DATA4, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPILCD_CLK, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPILCD_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPI_CLK, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPI_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_I2S_WS, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S3_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 2 3 */ {
                {HAL_IOMUX_FUNC_SDMMC_DATA5, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPILCD_CS, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPILCD_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPI_CS, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPI_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_I2S_SDI, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S3_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 2 4 */ {
                {HAL_IOMUX_FUNC_SDMMC_DATA6, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPILCD_DO, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPILCD_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPI_DO, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPI_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_I2S_SDO, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S3_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 2 5 */ {
                {HAL_IOMUX_FUNC_SDMMC_DATA7, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPILCD_DCN, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPILCD_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPI_DI, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPI_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_I2S_SCK, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_I2S3_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 2 6 */ {
                {HAL_IOMUX_FUNC_SDEMMC_DT_N, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_CLK_OUT, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_CLK_OUT_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPI_DI, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPI2_GPIO4_EN_MASK}, 
                {HAL_IOMUX_FUNC_SMP7, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SMP_EN_MASK}, 
                {HAL_IOMUX_FUNC_RF_ANA_SWRX, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_RF_ANA_SW2_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 2 7 */ {
                {HAL_IOMUX_FUNC_SDEMMC_WP, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SDMMC_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPILCD_DI, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPILCD_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },

    /* 3 0 */ {
                {HAL_IOMUX_FUNC_PWM0, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_PWM_LED_EN_0_MASK}, 
                {HAL_IOMUX_FUNC_BTUART_RX, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART2_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPDIF_DI, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPDIF_DIN_EN_MASK}, 
                {HAL_IOMUX_FUNC_UART0_RX, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART02_GPIO_EN_MASK}, 
                /* i2c is special, set i2c mode to 0 means enable i2c , 1 means gpio : other bits should be clear when used as i2c */
                {HAL_IOMUX_FUNC_I2C_SCL, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_I2C_GPIO_MODE_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 3 1 */ {
                {HAL_IOMUX_FUNC_PWM1, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_PWM_LED_EN_1_MASK}, 
                {HAL_IOMUX_FUNC_BTUART_TX, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART2_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_SPDIF_DO, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_SPDIF_DOUT_EN_MASK}, 
                {HAL_IOMUX_FUNC_UART0_TX, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART02_GPIO_EN_MASK}, 
                /* i2c is special, set i2c mode to 0 means enable i2c , 1 means gpio : other bits should be clear when used as i2c */
                {HAL_IOMUX_FUNC_I2C_SDA, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_I2C_GPIO_MODE_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 3 2 */ {
                {HAL_IOMUX_FUNC_PWM2, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_PWM_LED_EN_2_MASK}, 
                {HAL_IOMUX_FUNC_UART1_RX, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART1_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_REQ_26M, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_REQ_26M_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 3 3 */ {
                {HAL_IOMUX_FUNC_PWM3, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_PWM_LED_EN_3_MASK}, 
                {HAL_IOMUX_FUNC_UART1_TX, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART1_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_PTA_TX_CONF, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_PTA_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 3 4 */ {
                {HAL_IOMUX_FUNC_UART0_CTS, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART0_FLOW_EN_MASK}, 
                {HAL_IOMUX_FUNC_BTUART_CTS, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART2_FLOW_EN_MASK}, 
                {HAL_IOMUX_FUNC_PTA_STATUS, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_PTA_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 3 5 */ {
                {HAL_IOMUX_FUNC_UART0_RTS, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART0_FLOW_EN_MASK}, 
                {HAL_IOMUX_FUNC_BTUART_RTS, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART2_FLOW_EN_MASK}, 
                {HAL_IOMUX_FUNC_PTA_RF_ACT, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_PTA_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 3 6 */ {
                {HAL_IOMUX_FUNC_UART0_RX, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART0_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_RF_ANA_SWTX, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_RF_ANA_SW_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
    /* 3 7 */ {
                {HAL_IOMUX_FUNC_UART0_TX, IOMUXIP_MUX04_REG_OFFSET, IOMUXIP_MUX04_UART0_GPIO_EN_MASK}, 
                {HAL_IOMUX_FUNC_RF_ANA_SWRX, IOMUXIP_MUX10_REG_OFFSET, IOMUXIP_MUX10_RF_ANA_SW_EN_MASK}, 
                {HAL_IOMUX_FUNC_END, 0, 0},
              },
};

uint32_t _iomux_set_function_as_gpio(enum HAL_IOMUX_PIN_T pin)
{
    uint32_t i = 0, v = 0;
    for (i = 0; i < HAL_IOMUX_MAP_NUM; ++i) {
        if (_pfr_map[pin][i].func == HAL_IOMUX_FUNC_END)
            break;

        v = iomuxip_read32(IOMUXIP_REG_BASE, _pfr_map[pin][i].reg_offset);
#if defined(I2C_IS_SPECIAL)
        if (_pfr_map[pin][i].func != HAL_IOMUX_FUNC_I2C_SCL && _pfr_map[pin][i].func != HAL_IOMUX_FUNC_I2C_SDA) {
#endif
            v &= ~(_pfr_map[pin][i].bit_mask);
#if defined(I2C_IS_SPECIAL)
        }
        else {
            v |= _pfr_map[pin][i].bit_mask;
        }
#endif
        iomuxip_write32(v, IOMUXIP_REG_BASE, _pfr_map[pin][i].reg_offset);
    }
    return 0;
}

uint32_t _iomux_set_function_as_func(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_FUNCTION_T func)
{
    uint32_t i = 0, v = 0;
    /* i2c is special : i2c mode to 0 not 1 , 1 means gpio */
    for (i = 0; i < HAL_IOMUX_MAP_NUM; ++i) {
        if (_pfr_map[pin][i].func == HAL_IOMUX_FUNC_END)
            break;
        if (_pfr_map[pin][i].func == func) {
#if defined(I2C_IS_SPECIAL)
            if (func != HAL_IOMUX_FUNC_I2C_SCL && func != HAL_IOMUX_FUNC_I2C_SDA) {
#endif
                v = iomuxip_read32(IOMUXIP_REG_BASE, _pfr_map[pin][i].reg_offset);
                v |= _pfr_map[pin][i].bit_mask;
                iomuxip_write32(v, IOMUXIP_REG_BASE,_pfr_map[pin][i].reg_offset);
#if defined(I2C_IS_SPECIAL)
            }
            else {
                v = iomuxip_read32(IOMUXIP_REG_BASE, _pfr_map[pin][i].reg_offset);
                v &= ~(_pfr_map[pin][i].bit_mask);
                iomuxip_write32(v, IOMUXIP_REG_BASE,_pfr_map[pin][i].reg_offset);
            }
#endif
            break;
        }
    }
    return 0;
}

uint32_t hal_iomux_check(struct HAL_IOMUX_PIN_FUNCTION_MAP *map, uint32_t count)
{
    uint32_t i = 0;
    /* check if one pin to multi function */
    /* check if one function to multi pin : have meanings for input pin */
    /* TODO */
    for (i = 0; i < count; ++i) {
    }
    return 0;
}

uint32_t hal_iomux_init(struct HAL_IOMUX_PIN_FUNCTION_MAP *map, uint32_t count)
{
    uint32_t i = 0;
    for (i = 0; i < count; ++i) {
        hal_iomux_set_function(map[i].pin, map[i].function, HAL_IOMUX_OP_CLEAN_OTHER_FUNC_BIT);
		hal_iomux_set_io_voltage_domains(map[i].pin, map[i].volt);
		hal_iomux_set_io_pull_select(map[i].pin, map[i].pull_sel);
    }
    return 0;
}

uint32_t hal_iomux_set_function(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_FUNCTION_T func, enum HAL_IOMUX_OP_TYPE_T type)
{
    /* cancel all alt function is gpio function */
    if (func == HAL_IOMUX_FUNC_AS_GPIO || type == HAL_IOMUX_OP_CLEAN_OTHER_FUNC_BIT)
        _iomux_set_function_as_gpio(pin);

    if (func != HAL_IOMUX_FUNC_AS_GPIO)
        _iomux_set_function_as_func(pin, func);

    return 0;
}

enum HAL_IOMUX_FUNCTION_T hal_iomux_get_function(enum HAL_IOMUX_PIN_T pin)
{
    uint32_t v = 0, i = 0;
    enum HAL_IOMUX_FUNCTION_T func = HAL_IOMUX_FUNC_AS_GPIO;

    /* i2c is special : search for i2c func for this pin then read i2c mode */
    /* need _pfr_map put alt functions in priority order (see rtl) */

    for (i = 0; i < HAL_IOMUX_MAP_NUM; ++i) {
        if (_pfr_map[pin][i].func == HAL_IOMUX_FUNC_END)
            break;
        v = iomuxip_read32(IOMUXIP_REG_BASE, _pfr_map[pin][i].reg_offset);

#if defined(I2C_IS_SPECIAL)
        if (_pfr_map[pin][i].func == HAL_IOMUX_FUNC_I2C_SCL || _pfr_map[pin][i].func == HAL_IOMUX_FUNC_I2C_SDA) {
            /* v |0 == 0 means accorrding v bit is 0 */
            if ((v | ~(_pfr_map[pin][i].bit_mask)) == (~(_pfr_map[pin][i].bit_mask)))
                func = _pfr_map[pin][i].func;
        }
        else {
#endif
            /* v &1 == 1 means accorrding v bit is 1 */
            if (v & _pfr_map[pin][i].bit_mask)
                func = _pfr_map[pin][i].func;
#if defined(I2C_IS_SPECIAL)
        }
#endif
    }

    return func;
}

uint32_t hal_iomux_set_io_voltage_domains(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T volt)
{
    uint32_t v;

	//select sdmmc to vio
	v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX20_REG_OFFSET);
	
	if (volt)
		v |= (uint32_t)1 << pin;
	else
		v &= ~((uint32_t)1 << pin);
	iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX20_REG_OFFSET);

    return 0;
}

uint32_t hal_iomux_set_io_pull_select(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_PIN_PULL_SELECT_T pull_sel)
{
    uint32_t reg_08, reg_0C;

	reg_08 = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX08_REG_OFFSET);
	reg_08 &= ~((uint32_t)1 << pin);
	iomuxip_write32(reg_08, IOMUXIP_REG_BASE, IOMUXIP_MUX08_REG_OFFSET);
	
	reg_0C = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX0C_REG_OFFSET);
	reg_0C &= ~((uint32_t)1 << pin);
	iomuxip_write32(reg_0C, IOMUXIP_REG_BASE, IOMUXIP_MUX0C_REG_OFFSET);
	
	if (pull_sel == HAL_IOMUX_PIN_PULLUP_ENALBE){
		reg_08 |= (uint32_t)1 << pin;
		iomuxip_write32(reg_08, IOMUXIP_REG_BASE, IOMUXIP_MUX08_REG_OFFSET);
	}
	
	if (pull_sel == HAL_IOMUX_PIN_PULLDOWN_ENALBE){
		reg_0C |= (uint32_t)1 << pin;
		iomuxip_write32(reg_0C, IOMUXIP_REG_BASE, IOMUXIP_MUX0C_REG_OFFSET);
	}

    return 0;
}

void hal_iomux_set_uart0_p3_0(void)
{
    uint32_t v;

    v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX10_REG_OFFSET);
    v &= ~(IOMUXIP_MUX10_PWM_LED_EN_0_MASK | IOMUXIP_MUX10_PWM_LED_EN_1_MASK);
    v |= IOMUXIP_MUX10_I2C_GPIO_MODE_MASK;
    iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX10_REG_OFFSET);

    v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX04_REG_OFFSET);
#ifdef FPGA
    // MCU uses p3_6/7 pins so that BT can use p3_0/1 pins
    v |= IOMUXIP_MUX04_UART0_GPIO_EN_MASK;
    v &= ~IOMUXIP_MUX04_UART02_GPIO_EN_MASK;
#else
    v |= IOMUXIP_MUX04_UART02_GPIO_EN_MASK;
    v &= ~(IOMUXIP_MUX04_UART0_GPIO_EN_MASK | IOMUXIP_MUX04_UART2_GPIO_EN_MASK |
        IOMUXIP_MUX04_SPDIF_DIN_EN_MASK | IOMUXIP_MUX04_SPDIF_DOUT_EN_MASK);
#endif
    iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX04_REG_OFFSET);
}

void hal_iomux_set_uart1_p3_2(void)
{
    uint32_t v;

    v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX10_REG_OFFSET);
    v &= ~(IOMUXIP_MUX10_PWM_LED_EN_2_MASK | IOMUXIP_MUX10_PWM_LED_EN_3_MASK);
    iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX10_REG_OFFSET);

    v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX04_REG_OFFSET);
    v |= IOMUXIP_MUX04_UART1_GPIO_EN_MASK;
    iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX04_REG_OFFSET);
}

