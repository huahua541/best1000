#ifndef DRV_NORFLASH_H
#define DRV_NORFLASH_H

#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum DRV_NORFLASH_ERASE_TYPE {
    DRV_NORFLASH_ERASE_SECTOR = 0,
    DRV_NORFLASH_ERASE_BLOCK,
    DRV_NORFLASH_ERASE_CHIP,
};

enum DRV_NORFLASH_OP_MODE {
    /* (1) (2) (3) (4) can be used together, different flash-device may support different option(s) */

    /* (1) basic mode */
    DRV_NORFLASH_OP_MODE_STAND_SPI         = 0x1, /* standard spi mode */
    DRV_NORFLASH_OP_MODE_DUAL              = 0x2, /* dual mode */
    DRV_NORFLASH_OP_MODE_QUAD              = 0x4, /* quad mode */
    DRV_NORFLASH_OP_MODE_QPI               = 0x8, /* qpi mode */

    /* (2) read accelerate (no cmd bettween read operation) : may need Dual or Quad Mode */
    DRV_NORFLASH_OP_MODE_CONTINUOUS_READ   = 0x10,

    /* (3) read accelerate : fast clock */
    DRV_NORFLASH_OP_MODE_FAST              = 0x20,

    /* (40 output cmd and address accelerate (multi io pin) */
    DRV_NORFLASH_OP_MODE_OUTPUT            = 0x40,

    DRV_NORFLASH_OP_MODE_RESERVED          = 0xFFFFFFFF,
};

typedef uint8_t (* drv_norflash_pre_operation_t)(void);
typedef uint8_t (* drv_norflash_post_operation_t)(void);
typedef uint8_t (* drv_norflash_set_mode_t)(uint32_t mode, uint32_t op);
typedef uint8_t (* drv_norflash_match_chip_t)(void);
typedef uint8_t (* drv_norflash_get_status_t)(uint8_t *status_s0_s7, uint8_t *status_s8_s15);
typedef uint8_t (* drv_norflash_get_size_t)(uint32_t *total_size, uint32_t *block_size, uint32_t *sector_size, uint32_t *page_size);
typedef uint8_t (* drv_norflash_get_id_t)(uint8_t *value, uint32_t len);
typedef uint8_t (* drv_norflash_erase_t)(uint32_t start_address, enum DRV_NORFLASH_ERASE_TYPE type);
typedef uint8_t (* drv_norflash_write_t)(uint32_t start_address, const uint8_t *buffer, uint32_t len);
typedef uint8_t (* drv_norflash_read_t)(uint32_t start_address, uint8_t *buffer, uint32_t len);
typedef void (* drv_norflash_sleep_t)(void);
typedef void (* drv_norflash_wakeup_t)(void);

struct drv_norflash_ops {
    drv_norflash_pre_operation_t             drv_norflash_pre_operation;
    drv_norflash_post_operation_t            drv_norflash_post_operation;
    drv_norflash_set_mode_t                  drv_norflash_set_mode;
    drv_norflash_match_chip_t                drv_norflash_match_chip;
    drv_norflash_get_status_t                drv_norflash_get_status;
    drv_norflash_get_size_t                  drv_norflash_get_size;
    drv_norflash_get_id_t                    drv_norflash_get_id;
    drv_norflash_erase_t                     drv_norflash_erase;
    drv_norflash_write_t                     drv_norflash_write;
    drv_norflash_read_t                      drv_norflash_read;
    drv_norflash_sleep_t                     drv_norflash_sleep;
    drv_norflash_wakeup_t                    drv_norflash_wakeup;
};

#ifdef __cplusplus
}
#endif

#endif /* DRV_NORFLASH_H */
