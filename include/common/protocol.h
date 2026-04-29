#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <sys/types.h>

#define MAX_CMD 512

// Nome do pipe público do Controller
#define SERVER_FIFO "tmp/controller_fifo"

// Template para o pipe privado do Runner (ex: tmp/runner_fifo_1234)
#define CLIENT_FIFO_TEMPLATE "tmp/runner_fifo_%d"

typedef enum {
    MSG_REQUEST,    // Runner -> Controller (Pedido de execução)
    MSG_GO_AHEAD,   // Controller -> Runner (Podes começar!)
    MSG_DONE,       // Runner -> Controller (Terminei)
    MSG_QUERY,      // Runner -> Controller (Consultar estado -c)
    MSG_RESPONSE,   // Controller -> Runner (Resposta de texto)
    MSG_SHUTDOWN    // Runner -> Controller (Desligar -s)
} MsgType;

typedef struct {
    MsgType type;
    pid_t runner_pid;
    int user_id;
    int job_id;
    char command[MAX_CMD];
} Message;

#endif