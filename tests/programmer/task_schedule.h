#ifndef __TASK_SCHEDULE_H__
#define __TASK_SCHEDULE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"

#ifdef PROGRAMMER

#define TASK_SCHEDULE                   task_schedule()

int task_setup(uint8_t id, uint32_t sp, bool sleeping, void (*entry)(void));

bool task_schedule(void);

#else // PROGRAMMER

#define TASK_SCHEDULE                   true

#endif // PROGRAMMER

#ifdef __cplusplus
}
#endif

#endif

