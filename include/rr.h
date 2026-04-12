#ifndef RR_H
#define RR_H

#include "scheduler.h"

void rr_add_job(Scheduler *s, int job_id, int user_id, const char *command);
Job *rr_next_job(Scheduler *s);
void rr_list(Scheduler *s);
void rr_destroy(Scheduler *s);

#endif