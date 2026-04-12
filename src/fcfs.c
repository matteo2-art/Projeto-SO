#include <stdio.h>
#include <stdlib.h>
#include "../include/fcfs.h"

/* ── POLÍTICA FCFS (fila de espera) ──────────────────────────────────────── */

void fcfs_add_job(Scheduler *s, int job_id, int user_id, const char *command) {
    Job *job = make_job(job_id, user_id, command);
    if (job == NULL) { perror("job"); return; } 

    // Se a fila estiver completamente vazia, esta tarefa torna-se na primeira (head) e na última (tail)
    if (s->fcfs.head == NULL) {
        s->fcfs.head = job;
        s->fcfs.tail = job;
    } else {
        // Caso contrário, colamo-la atrás da pessoa que estava em último, e atualizamos o ponteiro da tail
        s->fcfs.tail->next = job;
        s->fcfs.tail       = job;
    }

    s->size++; // Incrementar o contador global de tarefas
}

Job *fcfs_next_job(Scheduler *s) {
    if (s->fcfs.head == NULL) return NULL; // A fila está vazia

    // Pegar na pessoa que está na frente da fila
    Job *job     = s->fcfs.head;
    
    // Mover a frente da fila para a próxima pessoa
    s->fcfs.head = job->next;
    
    // Se a fila estiver agora vazia, garantir que a tail não aponta para lixo
    if (s->fcfs.head == NULL) s->fcfs.tail = NULL;

    job->next = NULL; // Desconectar a tarefa da fila
    s->size--;
    return job;
}

void fcfs_list(Scheduler *s) {
    if (s->fcfs.head == NULL) { printf("sem tarefas pendentes\n"); return; }

    Job *job = s->fcfs.head;
    while (job != NULL) {
        printf("utilizador %d - tarefa %d - %s\n", job->user_id, job->job_id, job->command);
        job = job->next;
    }
}

void fcfs_destroy(Scheduler *s) {
    // Liberta (free) todas as tarefas na fila para evitar fugas de memória quando o controller se desligar
    Job *job = s->fcfs.head;
    while (job != NULL) {
        Job *tmp = job->next;
        free(job);
        job = tmp;
    }
    s->fcfs.head = NULL;
    s->fcfs.tail = NULL;
}