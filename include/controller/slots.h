#ifndef SLOTS_H
#define SLOTS_H

#include <sys/time.h>
#include "common/scheduler.h"
#include "common/protocol.h"

#define MAX_PARALLEL 64

/**
 * @brief RunningSlot — representa um slot de execução ativo
 *
 * @param active  Sinalizador (1 se ocupado, 0 se livre)
 * @param job_id  ID do job em execução
 * @param user_id ID do utilizador que submeteu o job
 * @param runner_pid PID do runner que executa o job
 * @param command Comando em execução
 * @param start_time Timestamp de quando o job foi autorizado a começar (first_run)
 * @param submit_time Timestamp de submissão (arrival)
 */

typedef struct {
    int            active;
    int            job_id;
    int            user_id;
    pid_t          runner_pid;
    char           command[MAX_CMD];
    struct timeval start_time;
    struct timeval submit_time; 
} RunningSlot;

/**
 * @brief Envia MSG_GO_AHEAD ao runner para começar a executar.
 *
 * @param runner_pid PID do runner
 * @param job_id ID do job a autorizar
 * @return void
 */
void send_go_ahead (pid_t runner_pid, int job_id);

/**
 * @brief Envia MSG_RESPONSE com texto (usado para -c e shutdown).
 *
 * @param runner_pid PID do runner que receberá a resposta
 * @param text Texto a enviar
 * @return void
 */
void send_response (pid_t runner_pid, const char *text);

/**
 * @brief Preenche um RunningSlot com o próximo job do scheduler.
 *
 * @param s Scheduler a consultar
 * @param slot Slot que será preenchido com os dados do job
 * @return void
 */
void dispatch_next (Scheduler *s, RunningSlot *slot);

/**
 * @brief Calcula duração e escreve uma linha no log.
 *
 * @param fd_log Descritor do ficheiro de log
 * @param slot Slot cujo job terminou
 * @param end Timestamp de fim (gettimeofday)
 * @return void
 */
void log_job (int fd_log, RunningSlot *slot, struct timeval *end);

/**
 * @brief Retorna índice de slot livre ou -1.
 *
 * @param slots Array de RunningSlot
 * @param parallel Número de slots paralelos
 * @return int índice do slot livre (>=0) ou -1 se nenhum livre
 */
int  find_free_slot(RunningSlot *slots, int parallel);

/**
 * @brief Verifica se todos os slots estão livres.
 *
 * @param slots Array de RunningSlot
 * @param parallel Número de slots paralelos
 * @return int 1 se todos livres, 0 caso contrário
 */
int  all_slots_free(RunningSlot *slots, int parallel);

#endif