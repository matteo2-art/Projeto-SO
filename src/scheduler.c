#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/scheduler.h"

/* ── SHARED HELPERS ──────────────────────────────────────── */

// Creates a new Job struct in memory
static Job *make_job(int job_id, int user_id, const char *command) {
    Job *job = malloc(sizeof(Job));
    if (job == NULL) { perror("malloc"); return NULL; }

    job->job_id  = job_id;
    job->user_id = user_id;
    
    // Safely copies the command string to ensure it doesn't overflow MAX_CMD
    strncpy(job->command, command, MAX_CMD - 1);
    job->command[MAX_CMD - 1] = '\0';
    job->next = NULL;
    return job;
}

/* ── FCFS POLICY (waiting line) ──────────────────────────────────────── */

static void fcfs_add_job(Scheduler *s, int job_id, int user_id, const char *command) {
    Job *job = make_job(job_id, user_id, command);
    if (job == NULL) { perror("job"); return; } 

    // If the line is completely empty, this job becomes both the front (head) and back (tail)
    if (s->fcfs.head == NULL) {
        s->fcfs.head = job;
        s->fcfs.tail = job;
    } else {
        // Otherwise, glue it behind the current last person, and update the tail pointer
        s->fcfs.tail->next = job;
        s->fcfs.tail       = job;
    }

    s->size++; // Increment our global job counter
}

static Job *fcfs_next_job(Scheduler *s) {
    if (s->fcfs.head == NULL) return NULL; // Queue is empty

    // Grab the person at the front of the line
    Job *job     = s->fcfs.head;
    
    // Move the front of the line to the next person
    s->fcfs.head = job->next;
    
    // If the line is now empty, make sure the tail doesn't point to garbage
    if (s->fcfs.head == NULL) s->fcfs.tail = NULL;

    job->next = NULL; // Disconnect the job from the line
    s->size--;
    return job;
}

static void fcfs_list(Scheduler *s) {
    if (s->fcfs.head == NULL) { printf("no pending jobs\n"); return; }

    Job *job = s->fcfs.head;
    while (job != NULL) {
        printf("user %d - job %d - %s\n", job->user_id, job->job_id, job->command);
        job = job->next;
    }
}

static void fcfs_destroy(Scheduler *s) {
    // Frees every job in the queue to prevent memory leaks when the controller shuts down
    Job *job = s->fcfs.head;
    while (job != NULL) {
        Job *tmp = job->next;
        free(job);
        job = tmp;
    }
    s->fcfs.head = NULL;
    s->fcfs.tail = NULL;
}

/* ── ROUND ROBIN POLICY (Circular list of users, each with their own line) ──────────────────────────────────────── */

// Searches the circular list to see if this user already has an active queue
static UserQueue *rr_find_user(Scheduler *s, int user_id) {
    if (s->rr.head == NULL) return NULL;

    UserQueue *uq = s->rr.head;
    // Use a do-while loop because it's a circle. Stop when wrap back around to head.
    do {
        if (uq->user_id == user_id) return uq;
        uq = uq->next;
    } while (uq != s->rr.head);

    return NULL;
}

// Adds a new user into the circular loop of turns
static void rr_insert_user(Scheduler *s, UserQueue *uq) {
    // If they are the first user ever, they point to themselves (forming a circle of 1)
    if (s->rr.head == NULL) {
        uq->next      = uq;
        s->rr.head    = uq;
        s->rr.current = uq;
        return;
    }

    // Find the current last user in the circle
    UserQueue *last = s->rr.head;
    while (last->next != s->rr.head) last = last->next;

    // Insert the new user at the end, and make them point back to the head to close the circle
    last->next = uq;
    uq->next   = s->rr.head;
}

// Removes a user from the circle (usually because they have no more jobs left)
static void rr_remove_user(Scheduler *s, UserQueue *uq) {
    // If they are the ONLY user in the circle
    if (uq->next == uq) {
        s->rr.head    = NULL;
        s->rr.current = NULL;
        free(uq);
        return;
    }

    // Find the user right before them, so we can bypass them
    UserQueue *prev = uq;
    while (prev->next != uq) prev = prev->next;

    // If we are deleting the user who is currently taking a turn, pass the baton first!
    if (s->rr.current == uq) s->rr.current = uq->next;
    
    // If we are deleting the anchor, move the anchor to the next guy
    if (s->rr.head    == uq) s->rr.head    = uq->next;

    // Bypass the user and delete them
    prev->next = uq->next;
    free(uq);
}

static void rr_add_job(Scheduler *s, int job_id, int user_id, const char *command) {
    Job *job = make_job(job_id, user_id, command);
    if (job == NULL) return;

    UserQueue *uq = rr_find_user(s, user_id);

    // If this user isn't in the circle yet, create a queue for them and insert them
    if (uq == NULL) {
        uq = malloc(sizeof(UserQueue));
        if (uq == NULL) { perror("malloc"); free(job); return; }

        uq->user_id = user_id;
        uq->head    = NULL;
        uq->tail    = NULL;
        uq->next    = NULL;
        rr_insert_user(s, uq);
    }

    // Add the job to this specific user's personal waiting line (standard FCFS logic)
    if (uq->head == NULL) {
        uq->head = job;
        uq->tail = job;
    } else {
        uq->tail->next = job;
        uq->tail       = job;
    }

    s->size++;
}

static Job *rr_next_job(Scheduler *s) {
    if (s->rr.current == NULL) return NULL; // No users have any jobs

    // 1. Give a turn: Grab the job at the front of the CURRENT user's line
    UserQueue *uq  = s->rr.current;
    Job       *job = uq->head;

    // 2. Pass the baton: Shift the 'current' pointer to the NEXT user in the circle
    s->rr.current = uq->next;

    // 3. Clean up: Remove the job from the user's line
    uq->head = job->next;
    if (uq->head == NULL) uq->tail = NULL;
    
    // If that was the user's very last job, kick the user out of the circular loop entirely
    if (uq->head == NULL) rr_remove_user(s, uq);

    job->next = NULL;
    s->size--;
    return job;
}

static void rr_list(Scheduler *s) {
    if (s->rr.head == NULL) { printf("no pending jobs\n"); return; }

    // Go through every user in the circle
    UserQueue *uq = s->rr.head;
    do {
        // Print every job in this user's personal line
        Job *job = uq->head;
        while (job != NULL) {
            printf("user %d - job %d - %s\n",
                   job->user_id, job->job_id, job->command);
            job = job->next;
        }
        uq = uq->next;
    } while (uq != s->rr.head);
}

static void rr_destroy(Scheduler *s) {
    if (s->rr.head == NULL) return;

    // Break the circle so we don't loop infinitely while freeing memory
    UserQueue *last = s->rr.head;
    while (last->next != s->rr.head) last = last->next;
    last->next = NULL;

    // Free all users and all their jobs
    UserQueue *uq = s->rr.head;
    while (uq != NULL) {
        Job *job = uq->head;
        while (job != NULL) {
            Job *tmp = job->next;
            free(job);
            job = tmp;
        }
        UserQueue *tmp = uq->next;
        free(uq);
        uq = tmp;
    }

    s->rr.head    = NULL;
    s->rr.current = NULL;
}

/* ── PUBLIC API (The "Traffic Directors") ──────────────────────────────────────── */

void scheduler_init(Scheduler *s, SchedPolicy policy) {
    // Set up the box, starting empty
    s->policy    = policy;
    s->size      = 0;
    
    // Initialize both compartments to NULL just to be safe
    s->fcfs.head = NULL;
    s->fcfs.tail = NULL;
    s->rr.head   = NULL;
    s->rr.current = NULL;
}

// These functions just look at the 'policy' switch and route to the correct helper function
void scheduler_add_job(Scheduler *s, int job_id, int user_id, const char *command) {
    if (s->policy == SCHED_FCFS)
        fcfs_add_job(s, job_id, user_id, command);
    else
        rr_add_job(s, job_id, user_id, command);
}

Job *scheduler_next_job(Scheduler *s) {
    if (s->policy == SCHED_FCFS)
        return fcfs_next_job(s);
    else
        return rr_next_job(s);
}

int scheduler_is_empty(Scheduler *s) {
    return s->size == 0;
}

void scheduler_list(Scheduler *s) {
    if (s->policy == SCHED_FCFS)
        fcfs_list(s);
    else
        rr_list(s);
}

void scheduler_destroy(Scheduler *s) {
    if (s->policy == SCHED_FCFS)
        fcfs_destroy(s);
    else
        rr_destroy(s);
}