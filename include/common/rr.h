#ifndef RR_H
#define RR_H
#include "common/scheduler.h"

void rr_add_job(Scheduler *s, int job_id, int user_id, pid_t runner_pid, const char *command);
Job *rr_next_job(Scheduler *s);
void rr_list(Scheduler *s, char *buf, int buf_size);
void rr_destroy(Scheduler *s);

#endif