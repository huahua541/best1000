#include "plat_types.h"
#include "drv_norflash.h"
#include "hal_norflaship.h"
#include "norflash_gd25q32c.h"
#include "hal_trace.h"
#include "task_schedule.h"

uint32_t gd25q32c_norflash_op_mode = 0;

//#define DUMP

/* private functions */

inline static uint32_t drv_norflash_read_status_s8_s15(void)
{
    uint32_t val = 0;
    norflaship_busy_wait();
    norflaship_clear_rxfifo();
    norflaship_busy_wait();
    norflaship_blksize(1);
	norflaship_write32(GD25Q32C_CMD_READ_STATUS_S8_S15<<8|TX_ADDR_RW_MASK, TX_CONFIG1_BASE);
    norflaship_rxfifo_count_wait(1);
    norflaship_busy_wait();
    val = norflaship_read_rxfifo();

    return val;
}

inline static uint32_t drv_norflash_read_status_s0_s7(void)
{
    uint32_t val = 0;
    norflaship_busy_wait();
    norflaship_clear_rxfifo();
    norflaship_busy_wait();
    norflaship_blksize(1);
	norflaship_write32(GD25Q32C_CMD_READ_STATUS_S0_S7<<8|TX_ADDR_RW_MASK, TX_CONFIG1_BASE);
    norflaship_rxfifo_count_wait(1);
    norflaship_busy_wait();
    val = norflaship_read_rxfifo();

    return val;
}

inline static uint8_t drv_norflash_status_WEL_1_wait(void)
{
    uint32_t status = 0;
    do {
        status = drv_norflash_read_status_s0_s7();
    } while (!(status&GD25Q32C_WEL_BIT_MASK) && TASK_SCHEDULE);

    return 0;
}

inline static uint8_t drv_norflash_status_WIP_1_wait(void)
{
    uint32_t status = 0;
    do {
        status = drv_norflash_read_status_s0_s7();
    } while ((status&GD25Q32C_WIP_BIT_MASK) && TASK_SCHEDULE);

    return 0;
}

inline static uint8_t drv_norflash_write_status_s8_s15(uint8_t status)
{
	norflaship_busy_wait();
	norflaship_clear_txfifo();
	norflaship_busy_wait();
	norflaship_write_txfifo(&status, 1);
	norflaship_write32(GD25Q32C_CMD_WRITE_STATUS_S8_S15<<8, TX_CONFIG1_BASE);
	norflaship_busy_wait();
	drv_norflash_status_WIP_1_wait();	

	return 0;
}

inline static uint8_t drv_norflash_write_status_s0_s7(uint8_t status)
{
    norflaship_busy_wait();
    norflaship_clear_txfifo();
    norflaship_busy_wait();
    norflaship_write_txfifo(&status, 1);
    norflaship_write32(GD25Q32C_CMD_WRITE_STATUS_S0_S7<<8, TX_CONFIG1_BASE);
    norflaship_busy_wait();
	drv_norflash_status_WIP_1_wait();

    return 0;
}

inline static uint8_t drv_norflash_status_QE_set(uint8_t enable)
{
    uint8_t status_s8_s15 = 0;

    status_s8_s15 = drv_norflash_read_status_s8_s15();
	
    if (enable)
        status_s8_s15 |= GD25Q32C_QE_BIT_MASK;
    else
        status_s8_s15 &= ~(GD25Q32C_QE_BIT_MASK);

    drv_norflash_write_status_s8_s15(status_s8_s15);

    status_s8_s15 = drv_norflash_read_status_s8_s15();

    return 0;
}

inline static uint8_t gd25q32c_drv_norflash_set_continuous_read(uint8_t on)
{
    if (on) {
        norflaship_busy_wait();
        norflaship_continuous_read_on();
        norflaship_busy_wait();

        /* M(5,4) = (1,0) means 'Contiguous Read' */
        norflaship_continuous_read_mode_bit(1<<5|0<<4);
        norflaship_busy_wait();
    }
    else {
        norflaship_busy_wait();
        norflaship_continuous_read_off();
        norflaship_busy_wait();

        /* M(5,4) = (1,0) means 'Contiguous Read' */
        norflaship_continuous_read_mode_bit(0);
        norflaship_busy_wait();

        /* FIXME, 0xFF is Disable QPI cmd, reset Contiguous Read here? */
        norflaship_cmd_addr(0xFF, 0);
        norflaship_busy_wait();
    }

    return 0;
}

inline static uint8_t gd25q32c_drv_norflash_set_quadmode(uint8_t on)
{
    if (on) {
        norflaship_cmd_addr(GD25Q32C_CMD_WRITE_ENABLE, 0);
        norflaship_busy_wait();
        drv_norflash_status_WIP_1_wait();
        drv_norflash_status_WEL_1_wait();
        drv_norflash_status_QE_set(1);
        drv_norflash_status_WIP_1_wait();

        norflaship_quad_mode(1);
        norflaship_hold_pin(1);
        norflaship_wpr_pin(1);
        norflaship_busy_wait();
    }
    else {
        norflaship_cmd_addr(GD25Q32C_CMD_WRITE_ENABLE, 0);
        norflaship_busy_wait();
        drv_norflash_status_WIP_1_wait();
        drv_norflash_status_WEL_1_wait();
        drv_norflash_status_QE_set(0);
        drv_norflash_status_WIP_1_wait();

        norflaship_quad_mode(0);
        norflaship_hold_pin(0);
        norflaship_wpr_pin(0);
        norflaship_busy_wait();
    }
    
    return 0;
}

/* driver api */
uint8_t gd25q32c_drv_norflash_read_status(uint8_t  *status_s0_s7, uint8_t  *status_s8_s15)
{

    norflaship_busy_wait();
    *status_s0_s7 = drv_norflash_read_status_s0_s7();
    *status_s8_s15 = drv_norflash_read_status_s8_s15();

    return 0;
}

uint8_t gd25q32c_drv_norflash_get_size(uint32_t *total_size, uint32_t *block_size, uint32_t *sector_size, uint32_t *page_size)
{
    if (total_size) {
        *total_size  = GD25Q32C_TOTAL_SIZE;
    }
    if (block_size) {
        *block_size  = GD25Q32C_BLOCK_SIZE;
    }
    if (sector_size) {
        *sector_size = GD25Q32C_SECTOR_SIZE;
    }
    if (page_size) {
        *page_size   = GD25Q32C_PAGE_SIZE;
    }

    return 0;
}
uint8_t gd25q32c_drv_norflash_set_mode(uint32_t mode, uint32_t op)
{
    gd25q32c_norflash_op_mode = mode;

    /* Quad Mode */
    if (op & DRV_NORFLASH_OP_MODE_QUAD) {
        /* on-> off */
        if (!(mode & DRV_NORFLASH_OP_MODE_QUAD)) {
            gd25q32c_drv_norflash_set_quadmode(0);
        }
        /* off -> on */
        else {
            gd25q32c_drv_norflash_set_quadmode(1);
        }
    }

    /* Contiguous Read */
    if (op & DRV_NORFLASH_OP_MODE_CONTINUOUS_READ) {
        /* (1) only support 'Contiguous Read' when Quad Mode or Dual Mode is on */
        if ((gd25q32c_norflash_op_mode & DRV_NORFLASH_OP_MODE_QUAD) || (gd25q32c_norflash_op_mode & DRV_NORFLASH_OP_MODE_DUAL)) {
            /*   on -> off */
            if (!(mode & DRV_NORFLASH_OP_MODE_CONTINUOUS_READ)) {
                gd25q32c_drv_norflash_set_continuous_read(0);
            }
            /*   off -> on */
            else {
                gd25q32c_drv_norflash_set_continuous_read(1);
            }
        }
    }

    return 0;
}
uint8_t gd25q32c_drv_norflash_pre_operation(void)
{
    if (gd25q32c_norflash_op_mode & DRV_NORFLASH_OP_MODE_CONTINUOUS_READ) {
        gd25q32c_drv_norflash_set_continuous_read(0);
    }

    return 0;
}
uint8_t gd25q32c_drv_norflash_post_operation(void)
{
    if (gd25q32c_norflash_op_mode & DRV_NORFLASH_OP_MODE_CONTINUOUS_READ) {
        gd25q32c_drv_norflash_set_continuous_read(1);
    }

    return 0;
}

uint8_t gd25q32c_drv_norflash_get_id(uint8_t *value, uint32_t len)
{
    gd25q32c_drv_norflash_pre_operation();

    norflaship_busy_wait();
    norflaship_clear_rxfifo();
    norflaship_busy_wait();

    norflaship_blksize(3);
    norflaship_cmd_addr(GD25Q32C_CMD_ID, 0);
    norflaship_busy_wait();
    norflaship_rxfifo_count_wait(3);

    value[0] = norflaship_read_rxfifo();
    value[1] = norflaship_read_rxfifo();
    value[2] = norflaship_read_rxfifo();

    gd25q32c_drv_norflash_post_operation();

#ifdef DUMP
    TRACE("GD25Q32C id : 0x%x-0x%x-0x%x\n", value[0], value[1], value[2]);
#endif

    return 0;
}
uint8_t gd25q32c_drv_norflash_match_chip(void)
{
    uint8_t id[3];
    gd25q32c_drv_norflash_get_id(id, 3);

#ifdef FPGA
    if (id[0] == 0xc8 && id[1] == 0x60 && id[2] == 0x16)
        return 1;
#else
    if (id[0] == 0xc8 && id[1] == 0x40 && id[2] == 0x16)
        return 1;
#endif

    return 0;
}
uint8_t gd25q32c_drv_norflash_erase(uint32_t start_address, enum DRV_NORFLASH_ERASE_TYPE type)
{
    norflaship_busy_wait();
    norflaship_cmd_addr(GD25Q32C_CMD_WRITE_ENABLE, start_address);
    norflaship_busy_wait(); // Need 1us. Or drv_norflash_status_WEL_1_wait(), which needs 6us.
    switch(type) {
        case DRV_NORFLASH_ERASE_SECTOR:
            norflaship_cmd_addr(GD25Q32C_CMD_SECTOR_ERASE, start_address);
            break;    
        case DRV_NORFLASH_ERASE_BLOCK:
            norflaship_cmd_addr(GD25Q32C_CMD_BLOCK_ERASE, start_address);
            break;    
        case DRV_NORFLASH_ERASE_CHIP:
            norflaship_cmd_addr(GD25Q32C_CMD_CHIP_ERASE, start_address);
            break;    
    }
    drv_norflash_status_WIP_1_wait();

    norflaship_busy_wait();
    return 0;
}
uint8_t gd25q32c_drv_norflash_write(uint32_t start_address, const uint8_t *buffer, uint32_t len)
{
    norflaship_busy_wait();
    norflaship_clear_txfifo();
    norflaship_write_txfifo(buffer, len);
    norflaship_busy_wait();
    norflaship_cmd_addr(GD25Q32C_CMD_WRITE_ENABLE, start_address);
    norflaship_busy_wait();
    if (gd25q32c_norflash_op_mode & DRV_NORFLASH_OP_MODE_QUAD) {
        norflaship_cmd_addr(GD25Q32C_CMD_QUAD_PAGE_PROGRAM, start_address);
    }
    /* not support Dual Write */
    //else if (gd25q32c_norflash_op_mode & DRV_NORFLASH_OP_MODE_DUAL) {
    //}
    else {
        norflaship_cmd_addr(GD25Q32C_CMD_PAGE_PROGRAM, start_address);
    }
    norflaship_busy_wait();

    drv_norflash_status_WIP_1_wait();

    return 0;
}
uint8_t gd25q32c_drv_norflash_read(uint32_t start_address, uint8_t *buffer, uint32_t len)
{
    uint32_t index = 0;
    norflaship_busy_wait();
    norflaship_clear_rxfifo();
    norflaship_busy_wait();
    /* read NORFLASHIP_ONCE_READ_SIZE (always is page size) , but fifo is NORFLASHIP_RXFIFO_SIZE_MAX (8 bytes) now */
    /* so we need to read when fifo is not empty, finally check total count */
    /* BUG : when read speed < spi in speed, lost read bytes, 
     * NORFLASHIP_ONCE_READ_SIZE is not ok always (only when clock is slow, not easy to handle), NORFLASHIP_RXFIFO_SIZE_MAX is OK always 
     * */
    norflaship_blksize(len);
    norflaship_busy_wait();
    if (gd25q32c_norflash_op_mode & DRV_NORFLASH_OP_MODE_QUAD) {
        /* Quad , only fast */
        norflaship_cmd_addr(GD25Q32C_CMD_FAST_QUAD_READ, start_address);
    }
    else if (gd25q32c_norflash_op_mode & DRV_NORFLASH_OP_MODE_DUAL) {
        /* Dual, only fast */
        norflaship_cmd_addr(GD25Q32C_CMD_FAST_DUAL_READ, start_address);
    }
    else {
        /* fast */
        if (gd25q32c_norflash_op_mode & DRV_NORFLASH_OP_MODE_FAST) {
            norflaship_cmd_addr(GD25Q32C_CMD_STANDARD_FAST_READ, start_address);
        }
        /* normal */
        else {
            norflaship_cmd_addr(GD25Q32C_CMD_STANDARD_READ, start_address);
        }
    }
    while(1) {
        norflaship_rxfifo_empty_wait();
        buffer[index] = norflaship_read_rxfifo();
        ++index;

        if(index >= len) {
            break;
        }
    }
    norflaship_busy_wait();

    return 0;
}

void gd25q32c_drv_norflash_sleep(void)
{
    gd25q32c_drv_norflash_pre_operation();
    norflaship_busy_wait();
    norflaship_cmd_addr(GD25Q32C_CMD_DEEP_POWER_DOWN, 0);
}

void gd25q32c_drv_norflash_wakeup(void)
{
    norflaship_busy_wait();
    norflaship_cmd_addr(GD25Q32C_CMD_RELEASE_FROM_DP, 0);
    gd25q32c_drv_norflash_post_operation();
}

struct drv_norflash_ops gd25q32c_ops = {
    gd25q32c_drv_norflash_pre_operation,
    gd25q32c_drv_norflash_post_operation,
    gd25q32c_drv_norflash_set_mode,
    gd25q32c_drv_norflash_match_chip,
    gd25q32c_drv_norflash_read_status,
    gd25q32c_drv_norflash_get_size,
    gd25q32c_drv_norflash_get_id,
    gd25q32c_drv_norflash_erase,
    gd25q32c_drv_norflash_write,
    gd25q32c_drv_norflash_read,
    gd25q32c_drv_norflash_sleep,
    gd25q32c_drv_norflash_wakeup,
};
