#ifndef FCFS_H
#define FCFS_H
#include "common/scheduler.h"

/**
 * @brief Adiciona um novo job ao fim da fila FCFS.
 *
 * @param s Scheduler onde adicionar o job
 * @param job_id Identificador do job
 * @param user_id Identificador do utilizador
 * @param runner_pid PID do runner que submeteu o job
 * @param command Comando a executar (string)
 * @return void
 */
void fcfs_add_job(Scheduler *s, int job_id, int user_id, pid_t runner_pid, const char *command);

/**
 * @brief Retira o job no início da fila (próximo a executar).
 *
 * @param s Scheduler de onde retirar o job
 * @return Job* ponteiro para o job removido (ou NULL se a fila estiver vazia)
 */
Job *fcfs_next_job(Scheduler *s);

/**
 * @brief Escreve uma representação textual da fila num buffer.
 *
 * @param s Scheduler a ler
 * @param buf Buffer de destino para o texto
 * @param buf_size Tamanho do buffer
 * @return void
 */
void fcfs_list(Scheduler *s, char *buf, int buf_size);

/**
 * @brief Liberta memória associada à fila FCFS.
 *
 * @param s Scheduler cuja fila será destruída
 * @return void
 */
void fcfs_destroy(Scheduler *s);

#endif