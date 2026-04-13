#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/scheduler.h"
#include "common/fcfs.h"
#include "common/rr.h"

/* ── FUNÇÕES AUXILIARES PARTILHADAS ──────────────────────────────────────── */

// Cria uma nova struct Job (Tarefa) na memória
Job *make_job(int job_id, int user_id, pid_t runner_pid, const char *command) {
    Job *job = malloc(sizeof(Job));
    if (job == NULL) { perror("malloc"); return NULL; }

    job->job_id     = job_id;
    job->user_id    = user_id;
    job->runner_pid = runner_pid;   
    strncpy(job->command, command, MAX_CMD - 1);
    job->command[MAX_CMD - 1] = '\0';
    job->next = NULL;
    return job;
}

/* ── API PÚBLICA (Os "Diretores de Trânsito") ──────────────────────────────────────── */

void scheduler_init(Scheduler *s, SchedPolicy policy) {
    // Configurar a caixa, começando vazia
    s->policy    = policy;
    s->size      = 0;
    
    // Inicializar ambos os compartimentos a NULL só por segurança
    s->fcfs.head = NULL;
    s->fcfs.tail = NULL;
    s->rr.head   = NULL;
    s->rr.current = NULL;
}

// Estas funções apenas olham para o switch da 'política' e reencaminham para a função auxiliar correta
void scheduler_add_job(Scheduler *s, int job_id, int user_id, pid_t runner_pid, const char *command) {
    if (s->policy == SCHED_FCFS)
        fcfs_add_job(s, job_id, user_id, runner_pid, command);
    else
        rr_add_job(s, job_id, user_id, runner_pid, command);
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

void scheduler_list(Scheduler *s, char *buf, int buf_size) {
    if (s->policy == SCHED_FCFS)
        fcfs_list(s, buf, buf_size);
    else
        rr_list(s, buf, buf_size);
}

void scheduler_destroy(Scheduler *s) {
    if (s->policy == SCHED_FCFS)
        fcfs_destroy(s);
    else
        rr_destroy(s);
}