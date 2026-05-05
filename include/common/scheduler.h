#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <sys/types.h>   
#include <sys/time.h>

#define MAX_CMD 512

// defines the policy
typedef enum {
    SCHED_FCFS = 0,
    SCHED_RR   = 1
} SchedPolicy;

// defines what a job is (shared in both policies)
typedef struct Job {
    int job_id;
    int user_id;
    pid_t runner_pid;
    char command[MAX_CMD];
    struct timeval submit_time;  // <- NOVO
    struct Job *next;
} Job;

// ── fcfs data ──────────────────────────────────────── 

typedef struct {
    Job *head;
    Job *tail;
} FCFSData;

// ── round robin data ───────────────────────────────── 

// defines the user's queue -> head and tail are the first and last user's job
typedef struct UserQueue {
    int user_id;
    Job *head;
    Job *tail;
    struct UserQueue *next;
} UserQueue;

// defines the big queue (circular queue) -> head is the first user and current is which user the queue is at the moment 
typedef struct {
    UserQueue *head;
    UserQueue *current;
} RRData;

//defines the Scheduler -> the policy, total number of waiting jobs (size), and the data inside
typedef struct {
    SchedPolicy policy;
    int size;
    FCFSData fcfs;
    RRData   rr;
} Scheduler;

Job *make_job          (int job_id, int user_id, pid_t runner_pid, const char *command);
void scheduler_init    (Scheduler *s, SchedPolicy policy);
void scheduler_add_job (Scheduler *s, int job_id, int user_id, pid_t runner_pid, const char *command);
Job *scheduler_next_job(Scheduler *s);
int  scheduler_is_empty(Scheduler *s);
void scheduler_list    (Scheduler *s, char *buf, int buf_size);
void scheduler_destroy (Scheduler *s);

#endif