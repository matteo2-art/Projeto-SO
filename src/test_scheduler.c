#include <stdio.h>
#include <stdlib.h>
#include "../include/scheduler.h"

static void run_test(SchedPolicy policy) {
    Scheduler s;
    scheduler_init(&s, policy);

    if (policy == SCHED_FCFS)
        printf("══ FCFS ══════════════════════════════\n");
    else
        printf("══ ROUND ROBIN ═══════════════════════\n");

    scheduler_add_job(&s, 1, 1, "echo user1 job1");
    scheduler_add_job(&s, 2, 2, "echo user2 job1");
    scheduler_add_job(&s, 3, 1, "echo user1 job2");
    scheduler_add_job(&s, 4, 3, "echo user3 job1");
    scheduler_add_job(&s, 5, 2, "echo user2 job2");

    printf("\n-- queue state --\n");
    scheduler_list(&s);

    printf("\n-- dispatch order --\n");
    while (!scheduler_is_empty(&s)) {
        Job *job = scheduler_next_job(&s);
        printf("dispatched: user %d - job %d - %s\n",
               job->user_id, job->job_id, job->command);
        free(job);
    }

    printf("\n-- queue after dispatch --\n");
    scheduler_list(&s);

    scheduler_destroy(&s);
    printf("\n");
}

int main() {
    run_test(SCHED_FCFS);
    run_test(SCHED_RR);
    return 0;
}