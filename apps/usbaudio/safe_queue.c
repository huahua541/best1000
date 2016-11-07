#include "cmsis.h"
#include "safe_queue.h"

void safe_queue_init(struct SAFE_QUEUE_T *queue, uint32_t *items, uint32_t size)
{
    queue->first = 0;
    queue->last = 0;
    queue->count = 0;
    queue->size = size;
    queue->items = items;
}

int safe_queue_put(struct SAFE_QUEUE_T *queue, uint32_t item)
{
    uint8_t idx;
    uint8_t cnt;

    do {
        if ((cnt = __LDREXB(&queue->count)) >= queue->size) {
            __CLREX();
            return 1;
        }
    } while (__STREXB(cnt + 1, &queue->count));

    do {
        cnt = (idx = __LDREXB(&queue->last)) + 1;
        if (cnt >= queue->size) {
            cnt = 0;
        }
    } while (__STREXB(cnt, &queue->last));

    queue->items[idx] = item;
    return 0;
}

int safe_queue_get(struct SAFE_QUEUE_T *queue, uint32_t *itemp)
{
    uint8_t idx;
    uint8_t cnt;

    do {
        if ((cnt = __LDREXB(&queue->count)) == 0) {
            __CLREX();
            return 1;
        }
    } while (__STREXB(cnt - 1, &queue->count));

    do {
        cnt = (idx = __LDREXB(&queue->first)) + 1;
        if (cnt >= queue->size) {
            cnt = 0;
        }
    } while (__STREXB(cnt, &queue->first));

    *itemp = queue->items[idx];
    return 0;
}

