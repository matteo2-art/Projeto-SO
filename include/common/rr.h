#ifndef RR_H
#define RR_H
#include "common/scheduler.h"

/**
 * @brief Adiciona um job à fila do respetivo utilizador (cria UserQueue se necessário).
 *
 * @param s Scheduler onde adicionar o job
 * @param job_id Identificador do job
 * @param user_id Identificador do utilizador
 * @param runner_pid PID do runner que submeteu o job
 * @param command Comando a executar (string)
 * @return void
 */
void rr_add_job(Scheduler *s, int job_id, int user_id, pid_t runner_pid, const char *command);

/**
 * @brief Devolve o próximo job usando a rotação entre utilizadores.
 *
 * @param s Scheduler de onde retirar o job
 * @return Job* ponteiro para o job removido (ou NULL se não houver jobs)
 */
Job *rr_next_job(Scheduler *s);

/**
 * @brief Escreve uma representação textual das filas por utilizador.
 *
 * @param s Scheduler a ler
 * @param buf Buffer de destino para o texto
 * @param buf_size Tamanho do buffer
 * @return void
 */
void rr_list(Scheduler *s, char *buf, int buf_size);

/**
 * @brief Liberta memória das filas e estruturas de utilizador.
 *
 * @param s Scheduler a destruir
 * @return void
 */
void rr_destroy(Scheduler *s);

#endif