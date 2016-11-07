#ifndef __SAFE_QUEUE_H__
#define __SAFE_QUEUE_H__

#include "plat_types.h"

struct SAFE_QUEUE_T {
    uint8_t first;
    uint8_t last;
    uint8_t count;
    uint8_t size;
    uint32_t *items;
};

void safe_queue_init(struct SAFE_QUEUE_T *queue, uint32_t *items, uint32_t size);

int safe_queue_put(struct SAFE_QUEUE_T *queue, uint32_t item);

int safe_queue_get(struct SAFE_QUEUE_T *queue, uint32_t *itemp);

#endif
