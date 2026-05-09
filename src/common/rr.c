#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "common/rr.h"

/* ── POLÍTICA ROUND ROBIN (Lista circular de utilizadores, cada um com a sua fila) ──────────────────────────────────────── */

// Procura na lista circular para ver se este utilizador já tem uma fila ativa
static UserQueue *rr_find_user(Scheduler *s, int user_id) {
    if (s->rr.head == NULL) return NULL;

    UserQueue *uq = s->rr.head;
    // Pára quando der a volta até voltar à head.
    while (1) {
        if (uq->user_id == user_id) return uq;

        uq = uq->next;

        if (uq == s->rr.head) break;
    }
    return NULL;
}

// Adiciona um novo utilizador ao ciclo circular de turnos
static void rr_insert_user(Scheduler *s, UserQueue *uq) {
    if (s->rr.head == NULL) {
        uq->next      = uq;
        s->rr.head    = uq;
        s->rr.current = uq;
        return;
    }
    UserQueue *last = s->rr.head;
    while (last->next != s->rr.head) last = last->next;
    last->next = uq;
    uq->next   = s->rr.head;
    /* avançar current para o novo utilizador —
     * garante que quem ainda não foi servido tem prioridade */
    s->rr.current = uq;
}

// Remove um utilizador do círculo (geralmente porque não lhe restam mais tarefas)
static void rr_remove_user(Scheduler *s, UserQueue *uq) {
    // Se for o único utilizador no círculo
    if (uq->next == uq) {
        s->rr.head    = NULL;
        s->rr.current = NULL;
        free(uq);
        return;
    }

    // Encontrar o utilizador imediatamente antes dele, para o poder contornar
    UserQueue *prev = uq;
    while (prev->next != uq) prev = prev->next;

    // Se apagar o utilizador que está atualmente no seu turno, passa-se o testemunho primeiro
    if (s->rr.current == uq) s->rr.current = uq->next;
    
    // Se estamos a apagar a head, move-se a para o próximo utilizador
    if (s->rr.head == uq) s->rr.head = uq->next;

    // Contornar o utilizador e apaga-o
    prev->next = uq->next;
    free(uq);
}

void rr_add_job(Scheduler *s, int job_id, int user_id, pid_t runner_pid, const char *command) {
    Job *job = make_job(job_id, user_id, runner_pid, command);
    if (job == NULL) return;

    UserQueue *uq = rr_find_user(s, user_id);
    if (uq == NULL) {
        uq = malloc(sizeof(UserQueue));
        if (uq == NULL) { perror("malloc"); free(job); return; }
        uq->user_id = user_id;
        uq->head    = NULL;
        uq->tail    = NULL;
        uq->next    = NULL;
        rr_insert_user(s, uq);
    }

    if (uq->head == NULL) {
        uq->head = job;
        uq->tail = job;
    } else {
        uq->tail->next = job;
        uq->tail       = job;
    }
    s->size++;
}

Job *rr_next_job(Scheduler *s) {
    if (s->rr.current == NULL) return NULL; // Nenhum utilizador tem tarefas

    // 1. Pegar na tarefa que está na frente da fila do utilizador atual
    UserQueue *uq  = s->rr.current;
    Job       *job = uq->head;

    // 2. Mover o ponteiro 'current' para o próximo utilizador no círculo
    s->rr.current = uq->next;

    // 3.Remover a tarefa da fila do utilizador
    uq->head = job->next;
    if (uq->head == NULL) uq->tail = NULL;
    
    // Se essa era a sua última tarefa, retirar o utilizador do ciclo circular 
    if (uq->head == NULL) rr_remove_user(s, uq);

    job->next = NULL;
    s->size--;
    return job;
}

void rr_list(Scheduler *s, char *buf, int buf_size) {
    if (s->rr.head == NULL) return;
    UserQueue *uq = s->rr.head;
    do {
        Job *job = uq->head;
        while (job != NULL) {
            char line[MAX_CMD + 64];
            snprintf(line, sizeof(line),
                "user-id %d - command-id %d\n", job->user_id, job->job_id);
            strncat(buf, line, buf_size - strlen(buf) - 1);
            job = job->next;
        }
        uq = uq->next;
    } while (uq != s->rr.head);
}

void rr_destroy(Scheduler *s) {
    if (s->rr.head == NULL) return;

    // Quebrar o círculo para não entar num loop infinito enquanto liberta-se memória
    UserQueue *last = s->rr.head;
    while (last->next != s->rr.head) last = last->next;
    last->next = NULL;

    // Libertar todos os utilizadores e todas as suas tarefas
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