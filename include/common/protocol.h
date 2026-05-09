#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <sys/types.h>

#define MAX_CMD 512

// Nome do pipe público do Controller
#define SERVER_FIFO "tmp/controller_fifo"

// Template para o pipe privado do Runner (ex: tmp/runner_fifo_1234)
#define CLIENT_FIFO_TEMPLATE "tmp/runner_fifo_%d"

/**
 * @brief Tipos de mensagem usados no protocolo entre runner e controller.
 *
 * @param MSG_REQUEST  Runner -> Controller: pedido de execução
 * @param MSG_GO_AHEAD Controller -> Runner: autorização para começar
 * @param MSG_DONE     Runner -> Controller: notifica conclusão
 * @param MSG_QUERY    Runner -> Controller: consulta de estado (-c)
 * @param MSG_RESPONSE Controller -> Runner: resposta de texto
 * @param MSG_SHUTDOWN Runner -> Controller: pedido de shutdown (-s)
 */
typedef enum {
    MSG_REQUEST,
    MSG_GO_AHEAD,
    MSG_DONE,
    MSG_QUERY,
    MSG_RESPONSE,
    MSG_SHUTDOWN
} MsgType;

/**
 * @brief Estrutura de mensagem fixa trocada via FIFOs.
 *
 * @param type       Tipo da mensagem (MsgType)
 * @param runner_pid PID do runner que enviou/recebeu a mensagem
 * @param user_id    ID do utilizador que submeteu o job
 * @param job_id     ID do job (quando aplicável)
 * @param command    Comando associado (string), se aplicável
 */
typedef struct {
    MsgType type;
    pid_t runner_pid;
    int user_id;
    int job_id;
    char command[MAX_CMD];
} Message;

#endif