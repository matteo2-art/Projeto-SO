#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/scheduler.h"
#include "../include/fcfs.h"
#include "../include/rr.h"

/* ── FUNÇÕES AUXILIARES PARTILHADAS ──────────────────────────────────────── */

// Cria uma nova struct Job (Tarefa) na memória
Job *make_job(int job_id, int user_id, const char *command) {
    Job *job = malloc(sizeof(Job));
    if (job == NULL) { perror("malloc"); return NULL; }

    job->job_id  = job_id;
    job->user_id = user_id;
    
    // Copia a string do comando com segurança para garantir que não ultrapassa o limite MAX_CMD
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