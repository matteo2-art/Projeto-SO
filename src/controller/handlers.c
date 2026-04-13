#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include "common/ipc.h"
#include "common/scheduler.h"
#include "common/protocol.h"
#include "controller/slots.h"
#include "controller/handlers.h"

void handle_request(Message *msg, Scheduler *s, RunningSlot *slots, int parallel, int *next_job_id) {
    int idx;

    scheduler_add_job(s, *next_job_id, msg->user_id, msg->runner_pid, msg->command);
    (*next_job_id)++;

    idx = find_free_slot(slots, parallel);
    if (idx >= 0)
        dispatch_next(s, &slots[idx]);
}

void handle_done(Message *msg, Scheduler *s, RunningSlot *slots, int parallel,
                 int fd_log, int shutdown_requested, pid_t shutdown_pid, int *should_exit) {
    struct timeval end_time;
    int i, idx;

    gettimeofday(&end_time, NULL);

    idx = -1;
    for (i = 0; i < parallel; i++) {
        if (slots[i].active && slots[i].runner_pid == msg->runner_pid) {
            idx = i;
            break;
        }
    }

    if (idx >= 0) {
        log_job(fd_log, &slots[idx], &end_time);
        slots[idx].active = 0;
        if (!scheduler_is_empty(s))
            dispatch_next(s, &slots[idx]);
    }

    if (shutdown_requested && all_slots_free(slots, parallel) && scheduler_is_empty(s)) {
        if (shutdown_pid > 0) send_response(shutdown_pid, "ok");
        *should_exit = 1;
    }
}

void handle_query(Message *msg, Scheduler *s, RunningSlot *slots, int parallel) {
    char buf[4096];
    int  i;

    buf[0] = '\0';
    strncat(buf, "---\nExecuting\n", sizeof(buf) - strlen(buf) - 1);
    for (i = 0; i < parallel; i++) {
        if (slots[i].active) {
            char line[MAX_CMD + 64];
            snprintf(line, sizeof(line),
                "user-id %d - command-id %d\n", slots[i].user_id, slots[i].job_id);
            strncat(buf, line, sizeof(buf) - strlen(buf) - 1);
        }
    }
    strncat(buf, "---\nScheduled\n", sizeof(buf) - strlen(buf) - 1);
    scheduler_list(s, buf, sizeof(buf));
    strncat(buf, "---\n",           sizeof(buf) - strlen(buf) - 1);

    send_response(msg->runner_pid, buf);
}

void handle_shutdown(Message *msg, RunningSlot *slots, int parallel, Scheduler *s,
                     pid_t *shutdown_pid, int *shutdown_requested, int *should_exit) {
    *shutdown_pid       = msg->runner_pid;
    *shutdown_requested = 1;

    if (all_slots_free(slots, parallel) && scheduler_is_empty(s)) {
        send_response(*shutdown_pid, "ok");
        *should_exit = 1;
    }
}