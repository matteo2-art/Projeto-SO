#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include "common/fcfs.h"

void fcfs_add_job(Scheduler *s, int job_id, int user_id, pid_t runner_pid, const char *command) {
    Job *job = make_job(job_id, user_id, runner_pid, command);
    if (job == NULL) { perror("job"); return; }

    if (s->fcfs.head == NULL) {
        s->fcfs.head = job;
        s->fcfs.tail = job;
    } else {
        s->fcfs.tail->next = job;
        s->fcfs.tail       = job;
    }
    s->size++;
}

Job *fcfs_next_job(Scheduler *s) {
    if (s->fcfs.head == NULL) return NULL; 

    Job *job     = s->fcfs.head;
    
    s->fcfs.head = job->next;
    
    if (s->fcfs.head == NULL) s->fcfs.tail = NULL;

    job->next = NULL; 
    s->size--;
    return job;
}

void fcfs_list(Scheduler *s, char *buf, int buf_size) {
    Job *job = s->fcfs.head;
    while (job != NULL) {
        char line[MAX_CMD + 64];
        snprintf(line, sizeof(line),
        "user-id %d - command-id %d\n", job->user_id, job->job_id);
        strncat(buf, line, buf_size - strlen(buf) - 1);
        job = job->next;
    }
}

void fcfs_destroy(Scheduler *s) {
    Job *job = s->fcfs.head;
    while (job != NULL) {
        Job *tmp = job->next;
        free(job);
        job = tmp;
    }
    s->fcfs.head = NULL;
    s->fcfs.tail = NULL;
}