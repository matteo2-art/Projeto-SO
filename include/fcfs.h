#ifndef FCFS_H
#define FCFS_H

#include "scheduler.h"

void fcfs_add_job(Scheduler *s, int job_id, int user_id, const char *command);
Job *fcfs_next_job(Scheduler *s);
void fcfs_list(Scheduler *s);
void fcfs_destroy(Scheduler *s);

#endif