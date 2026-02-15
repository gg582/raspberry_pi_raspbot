#ifndef LIBTTAK_SCHED_H
#define LIBTTAK_SCHED_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef void (*ttak_task_fn)(void* ctx);

typedef struct {
    ttak_task_fn fn;
    void* ctx;
    uint32_t interval_us;
    struct timespec next_run;
    bool active;
} ttak_task_t;

typedef struct {
    ttak_task_t* tasks;
    size_t task_count;
    size_t task_capacity;
} ttak_scheduler_t;

void ttak_sched_init(ttak_scheduler_t* sched, ttak_task_t* buffer, size_t capacity);
bool ttak_sched_add(ttak_scheduler_t* sched, ttak_task_fn fn, void* ctx, uint32_t interval_us);
void ttak_sched_run_once(ttak_scheduler_t* sched);
void ttak_sched_run_loop(ttak_scheduler_t* sched);

#endif // LIBTTAK_SCHED_H
