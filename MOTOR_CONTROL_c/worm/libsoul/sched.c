#define _POSIX_C_SOURCE 200809L

#include "sched.h"

#include <string.h>
#include <unistd.h>

static struct timespec now_monotonic(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

static struct timespec timespec_add_us(struct timespec ts, uint32_t us) {
    ts.tv_nsec += us * 1000ULL;
    while (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    return ts;
}

static bool timespec_greater(const struct timespec* a, const struct timespec* b) {
    if (a->tv_sec == b->tv_sec) {
        return a->tv_nsec > b->tv_nsec;
    }
    return a->tv_sec > b->tv_sec;
}

void ttak_sched_init(ttak_scheduler_t* sched, ttak_task_t* buffer, size_t capacity) {
    memset(buffer, 0, sizeof(ttak_task_t) * capacity);
    sched->tasks = buffer;
    sched->task_capacity = capacity;
    sched->task_count = 0;
}

bool ttak_sched_add(ttak_scheduler_t* sched, ttak_task_fn fn, void* ctx, uint32_t interval_us) {
    if (sched->task_count >= sched->task_capacity) {
        return false;
    }
    ttak_task_t* task = &sched->tasks[sched->task_count++];
    task->fn = fn;
    task->ctx = ctx;
    task->interval_us = interval_us;
    task->next_run = timespec_add_us(now_monotonic(), interval_us);
    task->active = true;
    return true;
}

void ttak_sched_run_once(ttak_scheduler_t* sched) {
    struct timespec current = now_monotonic();
    for (size_t i = 0; i < sched->task_count; ++i) {
        ttak_task_t* task = &sched->tasks[i];
        if (!task->active) {
            continue;
        }
        if (!timespec_greater(&task->next_run, &current)) {
            task->fn(task->ctx);
            task->next_run = timespec_add_us(task->next_run, task->interval_us);
            current = now_monotonic();
        }
    }
}

void ttak_sched_run_loop(ttak_scheduler_t* sched) {
    const struct timespec sleep_ts = {0, 1000 * 1000};
    while (true) {
        ttak_sched_run_once(sched);
        nanosleep(&sleep_ts, NULL);
    }
}
