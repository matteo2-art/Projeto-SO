#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "common/ipc.h"
#include "common/protocol.h"
#include "common/scheduler.h"
#include "controller/slots.h"

void send_go_ahead(pid_t runner_pid, int job_id) {
    char    path[64];
    Message msg;
    int     fd;

    sprintf(path, CLIENT_FIFO_TEMPLATE, runner_pid);
    msg.type       = MSG_GO_AHEAD;
    msg.runner_pid = runner_pid;
    msg.user_id    = 0;
    msg.job_id     = job_id; 
    msg.command[0] = '\0';
    fd = open_fifo_write(path);
    if (fd < 0) return;
    send_message(fd, &msg);
    close(fd);
}

void send_response(pid_t runner_pid, const char *text) {
    char    path[64];
    Message msg;
    int     fd;

    sprintf(path, CLIENT_FIFO_TEMPLATE, runner_pid);
    msg.type       = MSG_RESPONSE;
    msg.runner_pid = runner_pid;
    msg.user_id    = 0;
    strncpy(msg.command, text, MAX_CMD - 1);
    msg.command[MAX_CMD - 1] = '\0';
    fd = open_fifo_write(path);
    if (fd < 0) return;
    send_message(fd, &msg);
    close(fd);
}

void dispatch_next(Scheduler *s, RunningSlot *slot) {
    Job *job = scheduler_next_job(s);
    if (job == NULL) return;

    slot->active     = 1;
    slot->job_id     = job->job_id;
    slot->user_id    = job->user_id;
    slot->runner_pid = job->runner_pid;
    strncpy(slot->command, job->command, MAX_CMD - 1);
    slot->command[MAX_CMD - 1] = '\0';
    slot->submit_time = job->submit_time;
    gettimeofday(&slot->start_time, NULL);
    free(job);

    send_go_ahead(slot->runner_pid, slot->job_id);
}

void log_job(int fd_log, RunningSlot *slot, struct timeval *end) {
    char line[MAX_CMD + 256];
    long secs  = end->tv_sec  - slot->submit_time.tv_sec;
    long usecs = end->tv_usec - slot->submit_time.tv_usec;
    int  n;

    if (usecs < 0) { secs--; usecs += 1000000; }
    
    n = snprintf(line, sizeof(line),
        "job=%d user=%d cmd=%s arrival=%ld.%06ld first_run=%ld.%06ld duration=%ld.%06lds\n",
        slot->job_id, slot->user_id, slot->command,
        slot->submit_time.tv_sec, slot->submit_time.tv_usec,
        slot->start_time.tv_sec, slot->start_time.tv_usec,
        secs, usecs);
    write(fd_log, line, n);
}

int find_free_slot(RunningSlot *slots, int parallel) {
    int i;
    for (i = 0; i < parallel; i++)
        if (!slots[i].active) return i;
    return -1;
}

int all_slots_free(RunningSlot *slots, int parallel) {
    int i;
    for (i = 0; i < parallel; i++)
        if (slots[i].active) return 0;
    return 1;
}