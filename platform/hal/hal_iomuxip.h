#ifndef __HAL_IOMUXIP_H__
#define __HAL_IOMUXIP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "reg_iomuxip.h"

#define IOMUXIP_REG_BASE (0x4001F000)

#define iomuxip_read32(b,a) \
     (*(volatile uint32_t *)(b+a))

#define iomuxip_write32(v,b,a) \
     ((*(volatile uint32_t *)(b+a)) = v)

#ifdef __cplusplus
}
#endif

#endif /* __HAL_IOMUXIP_H__ */
