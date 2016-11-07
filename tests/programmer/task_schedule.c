#include "stddef.h"
#include "task_schedule.h"

extern void task_run(uint32_t task_id);

#define MAX_TASK_NUM            2

enum TASK_STATE_T {
    TASK_NONE,
    TASK_ACTIVE,
    TASK_SLEEPING,
};

uint32_t task_sp[MAX_TASK_NUM];

uint32_t cur_task_id = MAX_TASK_NUM;
static enum TASK_STATE_T task_state[MAX_TASK_NUM] = { TASK_NONE, TASK_NONE, };

bool task_schedule(void)
{
    uint32_t next_task_id;

    if (cur_task_id == MAX_TASK_NUM) {
        return true;
    }

    task_state[cur_task_id] = TASK_SLEEPING;
    next_task_id = cur_task_id;

    do {
        if (++next_task_id >= MAX_TASK_NUM) {
            next_task_id = 0;
        }
    } while (task_state[next_task_id] != TASK_SLEEPING);

    task_state[next_task_id] = TASK_ACTIVE;
    if (next_task_id != cur_task_id) {
        task_run(next_task_id);
    }

    return true;
}

int task_setup(uint8_t id, uint32_t sp, bool sleeping, void (*entry)(void))
{
    if (id >= MAX_TASK_NUM) {
        return 1;
    }
    if (sp & 0x7) {
        return 2;
    }
    if (task_state[id] != TASK_NONE) {
        return 3;
    }

    if (!sleeping) {
        if (cur_task_id != MAX_TASK_NUM) {
            return 4;
        }
        cur_task_id = id;
    }

    if (sp) {
        // r4-r12, lr
        task_sp[id] = sp - (10 * 4);
        // lr is located at the stack top
        *(uint32_t *)(sp - 4) = (uint32_t)entry;
    } else {
        task_sp[id] = 0;
    }

    task_state[id] = sleeping ? TASK_SLEEPING : TASK_ACTIVE;

    return 0;
}

