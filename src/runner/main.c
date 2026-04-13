#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "runner/parser.h"
#include "common/ipc.h"
#include "common/protocol.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        write(2, "uso: runner -e <user-id> <comando>...\n", 38);
        return 1;
    }

    pid_t my_pid = getpid();
    char my_fifo[64];
    sprintf(my_fifo, CLIENT_FIFO_TEMPLATE, my_pid);

    /* ── OPÇÃO: -e (Executar comando) ── */
    if (strcmp(argv[1], "-e") == 0 && argc >= 4) {
        int user_id = atoi(argv[2]);
        char command[MAX_CMD];
        join_args(command, MAX_CMD, argv, 3, argc);

        // 1. Criar pipe privado para a resposta
        create_fifo(my_fifo);

        // 2. Enviar pedido ao Controller
        Message msg = { .type = MSG_REQUEST, .runner_pid = my_pid, .user_id = user_id };
        strncpy(msg.command, command, MAX_CMD - 1);

        int fd_server = open_fifo_write(SERVER_FIFO);
        if (fd_server < 0) {
            unlink(my_fifo);
            return 1;
        }
        send_message(fd_server, &msg);
        close(fd_server);
        write(1, "[runner] comando submetido\n", 27);

        // 3. Esperar pelo OK do Controller no pipe privado
        int fd_private = open_fifo_read(my_fifo);
        Message res;
        receive_message(fd_private, &res);
        
        if (res.type == MSG_GO_AHEAD) {
            write(1, "[runner] a executar comando...\n", 31);
            execute_command(command);
            write(1, "[runner] comando terminado\n", 27);
        }

        // 4. Notificar que terminou
        fd_server = open_fifo_write(SERVER_FIFO);
        msg.type = MSG_DONE;
        send_message(fd_server, &msg);
        close(fd_server);

        // Limpeza
        unlink_fifo(fd_private, my_fifo);
        return 0;
    }

    /* ── OPÇÃO: -c (Consulta) ── */
    if (strcmp(argv[1], "-c") == 0) {
        create_fifo(my_fifo);
        Message msg = { .type = MSG_QUERY, .runner_pid = my_pid };
        
        int fd_server = open_fifo_write(SERVER_FIFO);
        send_message(fd_server, &msg);
        close(fd_server);

        int fd_private = open_fifo_read(my_fifo);
        // Aqui o runner ficaria a ler MSG_RESPONSE e a imprimir via write()
        write(1, "[runner] à espera da lista do controller...\n", 44);
        
        unlink_fifo(fd_private, my_fifo);
        return 0;
    }
    /* ── OPÇÃO: -s (Shutdown) ── */
    if (strcmp(argv[1], "-s") == 0) {
        create_fifo(my_fifo);

        Message msg;
        msg.type       = MSG_SHUTDOWN;
        msg.runner_pid = my_pid;
        msg.user_id    = 0;
        msg.command[0] = '\0';

        int fd_server = open_fifo_write(SERVER_FIFO);
        if (fd_server < 0) { unlink(my_fifo); return 1; }
        send_message(fd_server, &msg);
        close(fd_server);
        write(1, "[runner] sent shutdown notification\n", 36);

        /* esperar que o controller confirme que desligou */
        write(1, "[runner] waiting for controller to shutdown...\n", 47);
        int fd_private = open_fifo_read(my_fifo);
        Message res;
        receive_message(fd_private, &res);
        unlink_fifo(fd_private, my_fifo);

        write(1, "[runner] controller exited.\n", 28);
        return 0;
    }
}