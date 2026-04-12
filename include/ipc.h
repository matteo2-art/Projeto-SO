#ifndef IPC_H
#define IPC_H

#include "protocol.h"

// Cria um pipe (FIFO) no caminho especificado
int create_fifo(const char *path);

// Abre um pipe para escrita (bloqueia até haver um leitor)
int open_fifo_write(const char *path);

// Abre um pipe para leitura (bloqueia até haver um escritor)
int open_fifo_read(const char *path);

// Envia uma mensagem por um pipe já aberto
ssize_t send_message(int fd, Message *msg);

// Recebe uma mensagem de um pipe já aberto
ssize_t receive_message(int fd, Message *msg);

// Fecha o descritor e apaga o ficheiro do pipe
void unlink_fifo(int fd, const char *path);

#endif