#ifndef HANDLERS_H
#define HANDLERS_H

#include <sys/types.h>
#include "common/protocol.h"
#include "common/scheduler.h"
#include "controller/slots.h"

void handle_request (Message *msg, Scheduler *s, RunningSlot *slots, int parallel, int *next_job_id);
void handle_done    (Message *msg, Scheduler *s, RunningSlot *slots, int parallel,
                     int fd_log, int shutdown_requested, pid_t shutdown_pid, int *should_exit);
void handle_query   (Message *msg, Scheduler *s, RunningSlot *slots, int parallel);
void handle_shutdown(Message *msg, RunningSlot *slots, int parallel, Scheduler *s,
                     pid_t *shutdown_pid, int *shutdown_requested, int *should_exit);

#endif