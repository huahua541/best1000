#include "plat_types.h"
#include "hal_norflash.h"
#include "hal_norflaship.h"
#include "drv_norflash.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "task_schedule.h"
#include "string.h"

/* Demo:
 *
 *  uint8_t data[1024];
 *  hal_norflash_open(HAL_NORFLASH_ID_0, HAL_NORFLASH_SPEED_26M, 0);
 *  \/\/ hal_norflash_open(HAL_NORFLASH_ID_0, HAL_NORFLASH_SPEED_13M, DRV_NORFLASH_OP_MODE_QUAD);
 *  \/\/ hal_norflash_open(HAL_NORFLASH_ID_0, HAL_NORFLASH_SPEED_13M, DRV_NORFLASH_OP_MODE_QUAD|DRV_NORFLASH_OP_MODE_CONTINUOUS_READ);
 *  hal_norflash_erase(HAL_I2C_ID_0, 0, 4096);
 *  memset(data, 0xcc, 1024);
 *  hal_norflash_write(HAL_I2C_ID_0, 0, data, 1024);
 *  for (i = 0; i < 10; ++i) {
 *      TRACE("[0x%x] - 0x%x\n", 0x08000000 + i, *((volatile uint8_t *)(0x08000000 + i)));
 *  }
*/

#define HAL_NORFLASH_YES 1
#define HAL_NORFLASH_NO 0

extern uint32_t __flash_start[];

#if defined(PROGRAMMER)
extern struct drv_norflash_ops gd25q32c_ops;
extern struct drv_norflash_ops gd25q80c_ops;

/* add new flash driver here */
struct drv_norflash_ops *norflashs[] = {
    &gd25q80c_ops,
    &gd25q32c_ops,
    0,
};
#else
#if defined(__NORFLASH_GD25Q32C__)
extern struct drv_norflash_ops gd25q32c_ops;
struct drv_norflash_ops *norflashs[] = {
    &gd25q32c_ops,
    0,
};
#elif  defined(__NORFLASH_GD25Q80C__)
extern struct drv_norflash_ops gd25q80c_ops;
struct drv_norflash_ops *norflashs[] = {
    &gd25q80c_ops,
    0,
};
#endif
#endif

struct HAL_Norflash_Context norflash_ctx[HAL_NORFLASH_ID_NUM];

static const char * const invalid_drv = "norflash drv invalid";

static const struct HAL_NORFLASH_CONFIG_T norflash_cfg = {
#ifdef FPGA
    .source_clk = HAL_NORFLASH_SPEED_13M * 2,
    .speed = HAL_NORFLASH_SPEED_13M,
#elif defined(ULTRA_LOW_POWER)
    .source_clk = HAL_NORFLASH_SPEED_26M * 2,
    .speed = HAL_NORFLASH_SPEED_26M,
#else
    .source_clk = HAL_NORFLASH_SPEED_104M * 2,
    .speed = HAL_NORFLASH_SPEED_104M,
#endif
    .mode  = (enum DRV_NORFLASH_OP_MODE)(DRV_NORFLASH_OP_MODE_QUAD | DRV_NORFLASH_OP_MODE_CONTINUOUS_READ),
    .override_config = 0,
};

static enum HAL_CMU_FREQ_T hal_norflash_clk_to_cmu_freq(uint32_t clk)
{
    if (clk >= HAL_NORFLASH_SPEED_208M) {
        return HAL_CMU_FREQ_208M;
    } else if (clk >= HAL_NORFLASH_SPEED_104M) {
        return HAL_CMU_FREQ_104M;
    } else if (clk >= HAL_NORFLASH_SPEED_78M) {
        return HAL_CMU_FREQ_78M;
    } else if (clk >= HAL_NORFLASH_SPEED_52M) {
        return HAL_CMU_FREQ_52M;
    } else if (clk >= HAL_NORFLASH_SPEED_26M) {
        return HAL_CMU_FREQ_26M;
    } else {
        return HAL_CMU_FREQ_13M;
    }
}

/* hal api */
uint8_t hal_norflash_get_size(enum HAL_NORFLASH_ID_T id, uint32_t *total_size, 
        uint32_t *block_size, uint32_t *sector_size, uint32_t *page_size)
{
    ASSERT(norflash_ctx[id].drv_ops != 0, invalid_drv);
    return norflash_ctx[id].drv_ops->drv_norflash_get_size(total_size, block_size, sector_size, page_size);
}
uint8_t hal_norflash_get_boundary(enum HAL_NORFLASH_ID_T id, uint32_t address, uint32_t *block_boundary, uint32_t *sector_boundary)
{
    ASSERT(norflash_ctx[id].drv_ops != 0, invalid_drv);

                     /* block count                       * block size */
    if (block_boundary)
        *block_boundary  = (address/norflash_ctx[id].block_size)*norflash_ctx[id].block_size;
    if (sector_boundary)
        *sector_boundary = (address/norflash_ctx[id].sector_size)*norflash_ctx[id].sector_size;

    return 0;
}
uint8_t hal_norflash_get_id(enum HAL_NORFLASH_ID_T id, uint8_t *value, uint32_t len)
{
    ASSERT(norflash_ctx[id].drv_ops != 0, invalid_drv);
    return norflash_ctx[id].drv_ops->drv_norflash_get_id(value, len);
}
uint8_t hal_norflash_open(enum HAL_NORFLASH_ID_T id, const struct HAL_NORFLASH_CONFIG_T *cfg)
{
    uint32_t div = 0, i = 0;
    struct HAL_NORFLASH_CONFIG_T norcfg;

    // Place the config into ram
    memcpy(&norcfg, cfg, sizeof(norcfg));

    div = norcfg.source_clk/norcfg.speed;

    /* over write config */
    if (norcfg.override_config) {
        /* div */
        norflaship_div(norcfg.div);

        /* cmd quad */
        norflaship_cmdquad(norcfg.cmdquad?HAL_NORFLASH_YES:HAL_NORFLASH_NO);

        /* sample delay */
        norflaship_samdly(norcfg.samdly);

#if 0 /* removed in ip */
        /* dummy clc */
        norflaship_dummyclc(norcfg.dummyclc);

        /* dummy clc en */
        norflaship_dummyclcen(norcfg.dummyclcen);

        /* 4 byte address */
        norflaship_4byteaddr(norcfg.byte4byteaddr);
#endif

        /* ru en */
        norflaship_ruen(norcfg.spiruen);

        /* rd en */
        norflaship_rden(norcfg.spirden);

        /* rd cmd */
        norflaship_rdcmd(norcfg.rdcmd);

        /* frd cmd */
        norflaship_frdcmd(norcfg.frdcmd);

        /* qrd cmd */
        norflaship_qrdcmd(norcfg.qrdcmd);
    }
    else {
        norflaship_div(div);
    }

#ifdef SIMU
#ifdef SIMU_FAST_FLASH
#define MAX_SIMU_FLASH_FREQ     HAL_CMU_FREQ_104M
#else
#define MAX_SIMU_FLASH_FREQ     HAL_CMU_FREQ_52M
#endif
    if (hal_norflash_clk_to_cmu_freq(norcfg.source_clk) >= MAX_SIMU_FLASH_FREQ) {
        hal_cmu_flash_set_freq(MAX_SIMU_FLASH_FREQ);
        norflaship_div(2);
    }
    // Simulation requires sample delay 1
    norflaship_samdly(1);
#elif !defined(FPGA)
    if (norcfg.speed <= HAL_NORFLASH_SPEED_26M) {
        norflaship_samdly(1);
    } else {
        norflaship_samdly(2);
    }
#endif

    /* foreach driver in array, match chip and select drv_ops */
    norflash_ctx[id].drv_ops = 0;
#if defined(PROGRAMMER) || defined(__NORFLASH_GD25Q32C__) || defined(__NORFLASH_GD25Q80C__)
    for (i = 0; norflashs[i] != 0; ++i) {
        if (norflashs[i]->drv_norflash_match_chip && norflashs[i]->drv_norflash_match_chip()) {
            norflash_ctx[id].drv_ops = norflashs[i];
            break;
        }
    }
#endif

    if (norflash_ctx[id].drv_ops == 0)
        return 1;

    hal_norflash_get_size(id, &norflash_ctx[id].total_size, 
            &norflash_ctx[id].block_size, &norflash_ctx[id].sector_size, &norflash_ctx[id].page_size);

    hal_norflash_get_id(id, norflash_ctx[id].device_id, NORFLASH_DEVICE_ID_LEN_MAX);

    /* quad mode */ /* continuous read */
    norflash_ctx[id].drv_ops->drv_norflash_set_mode(norcfg.mode, DRV_NORFLASH_OP_MODE_QUAD|DRV_NORFLASH_OP_MODE_CONTINUOUS_READ);

    return 0;
}
uint8_t hal_norflash_erase_resume(enum HAL_NORFLASH_ID_T id)
{
    ASSERT(norflash_ctx[id].drv_ops != 0, invalid_drv);
    return 0;
}
uint8_t hal_norflash_erase_suspend(enum HAL_NORFLASH_ID_T id)
{
    ASSERT(norflash_ctx[id].drv_ops != 0, invalid_drv);
    return 0;
}
uint8_t hal_norflash_erase_chip(enum HAL_NORFLASH_ID_T id)
{
    uint32_t total_size = 0;
    total_size = norflash_ctx[id].total_size;

    hal_norflash_erase(id, 0, total_size);
    return 0;
}
uint8_t hal_norflash_erase(enum HAL_NORFLASH_ID_T id, uint32_t start_address, uint32_t len)
{
    uint32_t remain_len = 0, current_address = 0, total_size = 0, block_size = 0, sector_size = 0;

    //ASSERT(norflash_ctx[id].drv_ops != 0, invalid_drv);
    //ASSERT((len%norflash_ctx[id].sector_size) == 0, "norflash erase len \% sector_size != 0!");
    //ASSERT((start_address%norflash_ctx[id].sector_size) == 0, "norflash erase len \% sector_size != 0!");

    total_size      = norflash_ctx[id].total_size;
    block_size      = norflash_ctx[id].block_size;
    sector_size     = norflash_ctx[id].sector_size;
    current_address = start_address;

    remain_len      = (len%sector_size == 0)?len:(((len+sector_size)/sector_size)*sector_size);

    norflash_ctx[id].drv_ops->drv_norflash_pre_operation();

    while (remain_len > 0) {
        /* erase whole chip */
        if (remain_len == total_size) {
            norflash_ctx[id].drv_ops->drv_norflash_erase(current_address, DRV_NORFLASH_ERASE_CHIP);
            remain_len      -= total_size;
            current_address += total_size;
        }
        /* if large enough to erase a block and current_address is block boundary - erase a block */
        else if (remain_len >= block_size && ((current_address % block_size) == 0)) {
            norflash_ctx[id].drv_ops->drv_norflash_erase(current_address, DRV_NORFLASH_ERASE_BLOCK);
            remain_len      -= block_size;
            current_address += block_size;
        }
        /* erase a sector */
        else {
            norflash_ctx[id].drv_ops->drv_norflash_erase(current_address, DRV_NORFLASH_ERASE_SECTOR);
            remain_len      -= sector_size;
            current_address += sector_size;
        }
    }

    norflash_ctx[id].drv_ops->drv_norflash_post_operation();

    return 0;
}
uint8_t hal_norflash_write(enum HAL_NORFLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len)
{
    const uint8_t *current_buffer = 0;
    uint32_t remain_len = 0, current_address = 0, page_size = 0, write_size = 0;
    //ASSERT(norflash_ctx[id].drv_ops != 0, invalid_drv);
    //ASSERT((len%norflash_ctx[id].page_size) == 0, "norflash write length \% pagesize != 0\n");
    //ASSERT((start_address%norflash_ctx[id].page_size) == 0, "norflash write address \% pagesize != 0\n");

    remain_len      = len;
    page_size       = norflash_ctx[id].page_size;
    current_address = start_address;
    current_buffer  = buffer;

    norflash_ctx[id].drv_ops->drv_norflash_pre_operation();

    while (remain_len > 0) {

        if (remain_len >= page_size) {
            write_size = page_size;
        }
        else {
            write_size = remain_len;
        }

        norflash_ctx[id].drv_ops->drv_norflash_write(current_address, current_buffer, write_size);

        current_address += write_size;
        current_buffer  += write_size;
        remain_len      -= write_size;
    }

    norflash_ctx[id].drv_ops->drv_norflash_post_operation();

    return 0;
}
uint8_t hal_norflash_read(enum HAL_NORFLASH_ID_T id, uint32_t start_address, uint8_t *buffer, uint32_t len)
{
    uint8_t *current_buffer = 0;
    uint32_t remain_len = 0, current_address = 0, read_size = 0;
    //ASSERT(norflash_ctx[id].drv_ops != 0, invalid_drv);
    //ASSERT((len%norflash_ctx[id].page_size) == 0, "norflash read length \% NORFLASHIP_ONCE_READ_SIZE != 0\n");

    /* do NOT use NORFLASHIP_ONCE_READ_SIZE, see comments at gd25lq32c_drv_norflash_read function */
    read_size       = NORFLASHIP_RXFIFO_SIZE_MAX;
    remain_len      = len;
    current_address = start_address;
    current_buffer  = buffer;

    norflash_ctx[id].drv_ops->drv_norflash_pre_operation();

    while (remain_len > 0) {
        norflash_ctx[id].drv_ops->drv_norflash_read(current_address, current_buffer, read_size);

        current_address += read_size;
        current_buffer  += read_size;
        remain_len      -= read_size;
    }

    norflash_ctx[id].drv_ops->drv_norflash_post_operation();
    return 0;
}
uint8_t hal_norflash_close(enum HAL_NORFLASH_ID_T id)
{
    return 0;
}
void hal_norflash_sleep(enum HAL_NORFLASH_ID_T id)
{
    if (id < HAL_NORFLASH_ID_NUM && norflash_ctx[id].drv_ops &&
            norflash_ctx[id].drv_ops->drv_norflash_sleep) {
        norflash_ctx[id].drv_ops->drv_norflash_sleep();
    }
}
void hal_norflash_wakeup(enum HAL_NORFLASH_ID_T id)
{
    if (id < HAL_NORFLASH_ID_NUM && norflash_ctx[id].drv_ops &&
            norflash_ctx[id].drv_ops->drv_norflash_wakeup) {
        norflash_ctx[id].drv_ops->drv_norflash_wakeup();
    }
}
uint8_t hal_norflash_pre_operation(enum HAL_NORFLASH_ID_T id)
{
    *(volatile uint32_t *)__flash_start;
    norflash_ctx[id].drv_ops->drv_norflash_pre_operation();
    return 0;
}
uint8_t hal_norflash_post_operation(enum HAL_NORFLASH_ID_T id)
{
    *(volatile uint32_t *)__flash_start;
    norflash_ctx[id].drv_ops->drv_norflash_post_operation();
    return 0;
}
static void hal_norflash_prefetch_idle(void)
{
    hal_sys_timer_delay(4);
    if (norflaship_read32(INT_STATUS_BASE) & BUSY_MASK) {
        hal_sys_timer_delay(4);
    }
}
uint8_t hal_norflash_setup(enum HAL_NORFLASH_ID_T id, enum DRV_NORFLASH_OP_MODE mode)
{
    uint8_t status_s0_s7 = 0, status_s8_s15 = 0;

    uint32_t div = 0;
    struct HAL_NORFLASH_CONFIG_T norcfg = norflash_cfg;

    hal_cmu_flash_set_freq(hal_norflash_clk_to_cmu_freq(norcfg.source_clk));

    norcfg.mode = mode;

    div = norcfg.source_clk / norcfg.speed;

    norflaship_div(div);

#ifndef FPGA
    if (norcfg.speed <= HAL_NORFLASH_SPEED_26M) {
        norflaship_samdly(1);
    } else {
        norflaship_samdly(2);
    }
#endif
	norflash_ctx[id].drv_ops->drv_norflash_pre_operation();

    norflash_ctx[id].drv_ops->drv_norflash_get_status(&status_s0_s7, &status_s8_s15);

    /* quad mode */ /* continuous read */
    norflash_ctx[id].drv_ops->drv_norflash_set_mode(norcfg.mode, DRV_NORFLASH_OP_MODE_QUAD|DRV_NORFLASH_OP_MODE_CONTINUOUS_READ);

    norflash_ctx[id].drv_ops->drv_norflash_get_status(&status_s0_s7, &status_s8_s15);

	norflash_ctx[id].drv_ops->drv_norflash_post_operation();

    return 0;
}
uint8_t hal_norflash_init(void)
{
    int ret;

    hal_norflash_prefetch_idle();

    hal_cmu_flash_set_freq(hal_norflash_clk_to_cmu_freq(norflash_cfg.source_clk));

    ret = hal_norflash_open(HAL_NORFLASH_ID_0, &norflash_cfg);

    return ret;
}

