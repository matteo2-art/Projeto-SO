#ifndef SLOTS_H
#define SLOTS_H

#include <sys/time.h>
#include "common/scheduler.h"
#include "common/protocol.h"

#define MAX_PARALLEL 64

typedef struct {
    int            active;
    int            job_id;
    int            user_id;
    pid_t          runner_pid;
    char           command[MAX_CMD];
    struct timeval start_time;
} RunningSlot;

void send_go_ahead (pid_t runner_pid);
void send_response (pid_t runner_pid, const char *text);
void dispatch_next (Scheduler *s, RunningSlot *slot);
void log_job       (int fd_log, RunningSlot *slot, struct timeval *end);
int  find_free_slot(RunningSlot *slots, int parallel);
int  all_slots_free(RunningSlot *slots, int parallel);

#endif