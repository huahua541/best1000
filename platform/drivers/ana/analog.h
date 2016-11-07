#ifndef __ANALOG_H__
#define __ANALOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "hal_analogif.h"
#include "hal_cmu.h"
#include "hal_aud.h"

#define analog_read(reg,val)  hal_analogif_reg_read(reg,val)
#define analog_write(reg,val) hal_analogif_reg_write(reg,val)

#define ANALOG_REG31_OFFSET                         0x31
#define ANALOG_REG31_CODEC_RXEN_ADCA                (1 << 0)
#define ANALOG_REG31_CODEC_RXEN_ADCB                (1 << 1)
#define ANALOG_REG31_CODEC_RXEN_ADCC                (1 << 2)
#define ANALOG_REG31_CODEC_RXEN_ADCD                (1 << 3)
#define ANALOG_REG31_CODEC_TXEN_LDAC                (1 << 4)
#define ANALOG_REG31_CODEC_TXEN_RDAC                (1 << 5)
#define ANALOG_REG31_CODEC_CH0_MUTE                 (1 << 6)
#define ANALOG_REG31_CODEC_CH1_MUTE                 (1 << 7)
#define ANALOG_REG31_AUDPLL_FREQ_EN                 (1 << 8)
#define ANALOG_REG31_USBPLL_FREQ_EN                 (1 << 9)
#define ANALOG_REG31_CODEC_PGA1A_EN                 (1 << 10)
#define ANALOG_REG31_CODEC_PGA2A_EN                 (1 << 11)
#define ANALOG_REG31_CODEC_PGA1B_EN                 (1 << 12)
#define ANALOG_REG31_CODEC_PGA2B_EN                 (1 << 13)
#define ANALOG_REG31_CODEC_PGAC_EN                  (1 << 14)
#define ANALOG_REG31_CODEC_PGAD_EN                  (1 << 15)

#define ANALOG_REG31_ENABLE_ALL_DAC     (ANALOG_REG31_CODEC_TXEN_LDAC | ANALOG_REG31_CODEC_TXEN_RDAC)

#define ANALOG_REG31_ENABLE_MIC_A       (ANALOG_REG31_CODEC_RXEN_ADCA | ANALOG_REG31_CODEC_PGA1A_EN | ANALOG_REG31_CODEC_PGA2A_EN)
#define ANALOG_REG31_ENABLE_MIC_B       (ANALOG_REG31_CODEC_RXEN_ADCB | ANALOG_REG31_CODEC_PGA1B_EN | ANALOG_REG31_CODEC_PGA2B_EN)
#define ANALOG_REG31_ENABLE_MIC_C       (ANALOG_REG31_CODEC_RXEN_ADCC | ANALOG_REG31_CODEC_PGAC_EN)
#define ANALOG_REG31_ENABLE_MIC_D       (ANALOG_REG31_CODEC_RXEN_ADCD | ANALOG_REG31_CODEC_PGAD_EN)

#define ANALOG_REG31_ENABLE_LINE        (ANALOG_REG31_CODEC_RXEN_ADCA   | \
                                        ANALOG_REG31_CODEC_PGA2A_EN     | \
                                        ANALOG_REG31_CODEC_RXEN_ADCB    | \
                                        ANALOG_REG31_CODEC_PGA2B_EN)

#define ANALOG_REG31_ENABLE_ALL_ADC    (ANALOG_REG31_CODEC_RXEN_ADCA    | \
                                        ANALOG_REG31_CODEC_RXEN_ADCB    | \
                                        ANALOG_REG31_CODEC_RXEN_ADCC    | \
                                        ANALOG_REG31_CODEC_RXEN_ADCD    | \
                                        ANALOG_REG31_CODEC_PGA1A_EN     | \
                                        ANALOG_REG31_CODEC_PGA2A_EN     | \
                                        ANALOG_REG31_CODEC_PGA1B_EN     | \
                                        ANALOG_REG31_CODEC_PGA2B_EN     | \
                                        ANALOG_REG31_CODEC_PGAC_EN      | \
                                        ANALOG_REG31_CODEC_PGAD_EN)




#define ANALOG_REG32_OFFSET                         0x32
#define ANALOG_REG32_CFG_PGA1A_GAIN                   (1 << 0)
#define ANALOG_REG32_CFG_PGA1A_GAIN_SHIFT             (0)
#define ANALOG_REG32_CFG_PGA1A_GAIN_MASK              (0x7)
#define ANALOG_REG32_CFG_PGA2A_GAIN                     (1 << 3)
#define ANALOG_REG32_CFG_PGA2A_GAIN_SHIFT             (3)
#define ANALOG_REG32_CFG_PGA2A_GAIN_MASK              (0x7)
#define ANALOG_REG32_CFG_PGA1B_GAIN                  (1 << 6)
#define ANALOG_REG32_CFG_PGA1B_GAIN_SHIFT             (6)
#define ANALOG_REG32_CFG_PGA1B_GAIN_MASK              (0x7)
#define ANALOG_REG32_CFG_PGA2B_GAIN                        (1 << 9)
#define ANALOG_REG32_CFG_PGA2B_GAIN_SHIFT             (9)
#define ANALOG_REG32_CFG_PGA2B_GAIN_MASK              (0x7)
#define ANALOG_REG32_CFG_PGA1A_LARGEGAIN                      (1 << 12)
#define ANALOG_REG32_CFG_PGA1B_LARGEGAIN                (1 << 13)
#define ANALOG_REG32_CFG_PGAC_LARGEGAIN             (1 << 14)
#define ANALOG_REG32_CFG_PGAD_LARGEGAIN                (1 << 15)


#define ANALOG_REG34_OFFSET                         0x34
#define ANALOG_REG34_CFG_ADC_A                      (1 << 0)
#define ANALOG_REG34_CFG_ADC_A_DR                   (1 << 1)
#define ANALOG_REG34_CFG_ADC_B                      (1 << 2)
#define ANALOG_REG34_CFG_ADC_B_DR                   (1 << 3)
#define ANALOG_REG34_CFG_RESETN                     (1 << 4)
#define ANALOG_REG34_CFG_RESETN_DR                  (1 << 5)
#define ANALOG_REG34_CFG_RX                         (1 << 6)
#define ANALOG_REG34_CFG_RX_DR                      (1 << 7)
#define ANALOG_REG34_CFG_SARAD_RSTNC                (1 << 8)
#define ANALOG_REG34_CFG_SARAD_RSTNC_DR             (1 << 9)
#define ANALOG_REG34_CFG_SARAD_RSTND                (1 << 10)
#define ANALOG_REG34_CFG_SARAD_RSTND_DR             (1 << 11)
#define ANALOG_REG34_CFG_BBPLL1_PU                  (1 << 12)
#define ANALOG_REG34_CFG_BBPLL1_PU_DR               (1 << 13)
#define ANALOG_REG34_CFG_BBPLL2_PU                  (1 << 14)
#define ANALOG_REG34_CFG_BBPLL2_PU_DR               (1 << 15)


#define ANALOG_REG3A_OFFSET                         0x3A
#define ANALOG_REG3A_CFG_ADC_IBSEL                (1 << 0)
#define ANALOG_REGA1_CFG_ADC_IBSEL_SHIFT             (0)
#define ANALOG_REGA1_CFG_ADC_IBSEL_MASK              (0x7)
#define ANALOG_REG3A_CFG_ADC_VCM_CLOSE             (1 << 3)
#define ANALOG_REG3A_CFG_ADC_VREF_SEL                (1 << 4)
#define ANALOG_REGA1_CFG_ADC_VREF_SEL_SHIFT             (4)
#define ANALOG_REGA1_CFG_ADC_VREF_SEL_MASK              (0xf)
#define ANALOG_REG3A_CFG_BBOLL1_IQCAL_TTG_EN             (1 << 8)
#define ANALOG_REG3A_CFG_BBPLL1_CLKGEN_EN                (1 << 9)
#define ANALOG_REG3A_CFG_BBPLL1_CLKGEN_DIVN              (1 << 10)
#define ANALOG_REGA1_CFG_BBPLL1_CLKGEN_DIVN_SHIFT             (10)
#define ANALOG_REGA1_CFG_BBPLL1_CLKGEN_DIVN_MASK              (0xf)

#define ANALOG_REG3A_CFG_TXEN_LPA                  (1 << 14)
#define ANALOG_REG3A_CFG_TXEN_RPA              (1 << 15)

#define ANALOG_REG3A_ENABLE_ALL_DAC_PA     (ANALOG_REG3A_CFG_TXEN_LPA | ANALOG_REG3A_CFG_TXEN_RPA)

#define ANALOG_REG3B_OFFSET                         0x3b
#define ANALOG_REG3B_CFG_TX_EN_LPPA                 (1 << 0)
#define ANALOG_REG3B_CFG_TX_EN_LPPA_DR              (1 << 1)
#define ANALOG_REG3B_CFG_TX_EN_S1PA                 (1 << 2)
#define ANALOG_REG3B_CFG_TX_EN_S1PA_DR              (1 << 3)
#define ANALOG_REG3B_CFG_TX_EN_S2PA                 (1 << 4)
#define ANALOG_REG3B_CFG_TX_EN_S2PA_DR              (1 << 5)
#define ANALOG_REG3B_CFG_TX_EN_S3PA                 (1 << 6)
#define ANALOG_REG3B_CFG_TX_EN_S3PA_DR              (1 << 7)
#define ANALOG_REG3B_CODEC_TX_PA_DIFFMODE           (1 << 8)
#define ANALOG_REG3B_CODEC_TX_PA_DOUBLEBIAS         (1 << 9)
#define ANALOG_REG3B_CODEC_TX_PA_SOFTSTART_SHIFT    (10)
#define ANALOG_REG3B_CODEC_TX_PA_SOFTSTART_MASK     (0x3f)
#define ANALOG_REG3B_CODEC_TX_PA_SOFTSTART(n)       (((n) & ANALOG_REG3B_CODEC_TX_PA_SOFTSTART_MASK) << ANALOG_REG3B_CODEC_TX_PA_SOFTSTART_SHIFT)

#define ANALOG_REGA1_OFFSET                         0xa1
#define ANALOG_REGA1_PGAC_CAPMODE                   (1 << 0)
#define ANALOG_REGA1_PGAC_FASTSETTLE                (1 << 1)
#define ANALOG_REGA1_PGAD_CAPMODE                   (1 << 2)
#define ANALOG_REGA1_PGAD_FASTSETTLE                (1 << 3)
#define ANALOG_REGA1_PGA2A_LOWNOISE                 (1 << 4)
#define ANALOG_REGA1_EN_VCMLPF                      (1 << 5)
#define ANALOG_REGA1_CFG_TX_EN_S3PA                 (1 << 6)
#define ANALOG_REGA1_CFG_TX_EN_S3PA_DR              (1 << 7)
#define ANALOG_REGA1_CFG_DIV_100K_SHIFT             (8)
#define ANALOG_REGA1_CFG_DIV_100K_MASK              (0xff)
#define ANALOG_REGA1_CFG_DIV_100K(n)                (((n) & ANALOG_REGA1_CFG_DIV_100K_MASK) << ANALOG_REGA1_CFG_DIV_100K_SHIFT)

void analog_aud_48000_pll_config(void);

void analog_aud_44100_pll_config(void);

void analog_aud_pll_config(enum AUD_SAMPRATE_T samprate, int shift);

void analog_aud_freq_pll_config(enum HAL_CMU_CODEC_FREQ_T freq,enum AUD_SAMPRATE_T samprate);

void analog_aud_pll_tune(int sample, uint32_t ms, int adc);

void anlog_open(void);

int analog_usbdevice_init(void);

int analog_usbhost_init(void);

void analog_aud_pll_open(void);

void analog_aud_pll_close(void);

void analog_aud_input_cfg(enum AUD_IO_PATH_T input_path);

void analog_codec_tx_pa_gain_sel(uint32_t v);

void analog_aud_coedc_open(enum AUD_IO_PATH_T input_path);

void analog_aud_coedc_close(void);

void analog_aud_coedc_mute(void);

void analog_aud_coedc_nomute(void);

void analog_aud_codec_mic_enable(bool en);

void analog_aud_codec_speaker_enable(bool en);

#ifdef __cplusplus
}
#endif

#endif

