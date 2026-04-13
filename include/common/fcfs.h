#ifndef FCFS_H
#define FCFS_H
#include "common/scheduler.h"

void fcfs_add_job(Scheduler *s, int job_id, int user_id, pid_t runner_pid, const char *command);
Job *fcfs_next_job(Scheduler *s);
void fcfs_list(Scheduler *s, char *buf, int buf_size);
void fcfs_destroy(Scheduler *s);

#endif