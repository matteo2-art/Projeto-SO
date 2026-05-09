#ifndef IPC_H
#define IPC_H

#include "common/protocol.h"

/**
 * @brief Cria um pipe (FIFO) no caminho especificado.
 *
 * @param path Caminho do FIFO a criar
 * @return int 0 em sucesso, -1 em erro
 */
int create_fifo(const char *path);

/**
 * @brief Abre um pipe para escrita (bloqueia até haver um leitor).
 *
 * @param path Caminho do FIFO a abrir para escrita
 * @return int descritor de ficheiro (fd) >= 0 em sucesso, -1 em erro
 */
int open_fifo_write(const char *path);

/**
 * @brief Abre um pipe para leitura (bloqueia até haver um escritor).
 *
 * @param path Caminho do FIFO a abrir para leitura
 * @return int descritor de ficheiro (fd) >= 0 em sucesso, -1 em erro
 */
int open_fifo_read(const char *path);

/**
 * @brief Envia uma mensagem por um pipe já aberto.
 *
 * @param fd  descritor do pipe para escrita
 * @param msg ponteiro para a mensagem a enviar
 * @return ssize_t número de bytes escritos (sizeof(Message)) ou -1 em erro
 */
ssize_t send_message(int fd, Message *msg);

/**
 * @brief Recebe uma mensagem de um pipe já aberto.
 *
 * @param fd  descritor do pipe para leitura
 * @param msg ponteiro para struct Message que será preenchida
 * @return ssize_t número de bytes lidos (sizeof(Message)) ou -1 em erro
 */
ssize_t receive_message(int fd, Message *msg);

/**
 * @brief Fecha o descritor e apaga o ficheiro do pipe (unlink).
 *
 * @param fd   descritor de ficheiro a fechar
 * @param path caminho do FIFO a apagar
 * @return void
 */
void unlink_fifo(int fd, const char *path);

#endif