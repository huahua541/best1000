/* psram calib */
#include "hal_uart.h"
#include "hal_psram.h"
#include "hal_trace.h"

#define REG(a) \
    *((volatile uint32_t *)a)

#define WR_NUM (8)
#define RD_NUM (8)

uint32_t test_pt[] = {
    0x4a174b18,
    0xe92d41f0,
    0xf8525020,
    0xeb031840,
    0xaa5555aa,
};

char psram_calib_res[8][8] = {0};

void psram_calib(void)
{
    uint32_t wr_sel = 0, rd_sel = 0, i = 0;
    struct HAL_PSRAM_CONFIG_T psram_cfg;

    psram_cfg.override_config = 0;
    //psram_cfg.speed = HAL_PSRAM_SPEED_26M;
    psram_cfg.speed = HAL_PSRAM_SPEED_50M;

    TRACE("psram calib start...\n");

    for (wr_sel = 0; wr_sel < WR_NUM; ++wr_sel) {
        for (rd_sel = 0; rd_sel < RD_NUM; ++rd_sel) {
            psram_cfg.dqs_rd_sel = rd_sel;
            psram_cfg.dqs_wr_sel = wr_sel;

            TRACE("wr_sel - %d, rd_sel - %d\n", wr_sel, rd_sel);
            hal_psram_open(HAL_PSRAM_ID_0, &psram_cfg);

            /* sequence write */
            for (i = 0; i < (sizeof(test_pt)/sizeof(uint32_t)); ++i) {
            REG(0x18000000) = test_pt[i];

            if (REG(0x18000000) != test_pt[i]) {
                    TRACE("err 0x%x\n", test_pt[i]);
                    psram_calib_res[wr_sel][rd_sel] = 0;
                    break;
             }
            }

            if (i == (sizeof(test_pt)/sizeof(uint32_t))) {
                TRACE("ok\n\n", test_pt[i]);
                psram_calib_res[wr_sel][rd_sel] = 1;
            }
        }
    }

    TRACE("psram calib done\n");
}

__PSRAMCODE void psram_calib_test(void)
{
    uint32_t wr_sel = 0, rd_sel = 0, i = 0;
    struct HAL_PSRAM_CONFIG_T psram_cfg;

    psram_cfg.override_config = 0;
    psram_cfg.speed = HAL_PSRAM_SPEED_26M;

    TRACE("psram calib start...\n");

    for (wr_sel = 0; wr_sel < WR_NUM; ++wr_sel) {
        for (rd_sel = 0; rd_sel < RD_NUM; ++rd_sel) {
            psram_cfg.dqs_rd_sel = rd_sel;
            psram_cfg.dqs_wr_sel = wr_sel;

            TRACE("wr_sel - %d, rd_sel - %d\n", wr_sel, rd_sel);
            //hal_psram_open(HAL_PSRAM_ID_0, &psram_cfg);

            /* sequence write */
            for (i = 0; i < (sizeof(test_pt)/sizeof(uint32_t)); ++i) {
            //    REG(0x18000000) = test_pt[i];

             //   if (REG(0x18000000) != test_pt[i]) {
                    TRACE("err 0x%x\n", test_pt[i]);
                    psram_calib_res[wr_sel][rd_sel] = 0;
                    break;
             //   }
            }

            if (i == (sizeof(test_pt)/sizeof(uint32_t))) {
                TRACE("ok\n\n", test_pt[i]);
                psram_calib_res[wr_sel][rd_sel] = 1;
            }
        }
    }

    TRACE("psram calib done\n");
}
