#ifndef HANDLERS_H
#define HANDLERS_H

#include <sys/types.h>
#include "common/protocol.h"
#include "common/scheduler.h"
#include "controller/slots.h"

/**
 * @brief Processa MSG_REQUEST: adiciona job ao scheduler e despacha se houver slot livre.
 *
 * @param msg Ponteiro para a mensagem recebida (MSG_REQUEST)
 * @param s Scheduler onde o job será inserido
 * @param slots Array de `RunningSlot` representando os slots ativos
 * @param parallel Número de slots paralelos configurados
 * @param next_job_id Ponteiro para o contador do próximo job_id
 * @return void
 */
void handle_request (Message *msg, Scheduler *s, RunningSlot *slots, int parallel, int *next_job_id);

/**
 * @brief Processa MSG_DONE: regista o job no log, liberta slot e despacha o próximo job se houver.
 *
 * @param msg Mensagem MSG_DONE recebida do runner
 * @param s Scheduler utilizado para obter o próximo job
 * @param slots Array de `RunningSlot` onde o job estava a correr
 * @param parallel Número de slots paralelos
 * @param fd_log Descritor do ficheiro de log (log.txt)
 * @param shutdown_requested Flag indicando se foi pedido shutdown
 * @param shutdown_pid PID do processo que pediu shutdown (para responder)
 * @param should_exit Ponteiro para sinalizar que o controller deve terminar
 * @return void
 */
void handle_done (Message *msg, Scheduler *s, RunningSlot *slots, int parallel,
                  int fd_log, int shutdown_requested, pid_t shutdown_pid, int *should_exit);

/**
 * @brief Processa MSG_QUERY: responde com o estado atual de execução e da fila.
 *
 * @param msg Mensagem MSG_QUERY (o runner que pediu recebe a resposta)
 * @param s Scheduler para ler a fila agendada
 * @param slots Array de `RunningSlot` para listar os jobs em execução
 * @param parallel Número de slots paralelos
 * @return void
 */
void handle_query (Message *msg, Scheduler *s, RunningSlot *slots, int parallel);

/**
 * @brief Processa MSG_SHUTDOWN: marca pedido de shutdown e inicia sequência se possível.
 *
 * @param msg Mensagem MSG_SHUTDOWN recebida
 * @param slots Array de `RunningSlot` para verificar se existem execuções em curso
 * @param parallel Número de slots paralelos
 * @param s Scheduler para verificar se a fila está vazia
 * @param shutdown_pid Ponteiro para gravar o PID do pedido de shutdown
 * @param shutdown_requested Ponteiro para sinalizar que existe pedido de shutdown
 * @param should_exit Ponteiro para sinalizar que o controller deve terminar quando possível
 * @return void
 */
void handle_shutdown (Message *msg, RunningSlot *slots, int parallel, Scheduler *s,
                      pid_t *shutdown_pid, int *shutdown_requested, int *should_exit);

#endif