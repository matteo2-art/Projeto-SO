#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "runner/parser.h"
#include "common/ipc.h"
#include "common/protocol.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        write(2, "uso: runner -e <user-id> <comando>...\n"
                 "     runner -c\n"
                 "     runner -s\n", 68);
        return 1;
    }

    pid_t my_pid = getpid();
    char  my_fifo[64];
    sprintf(my_fifo, CLIENT_FIFO_TEMPLATE, my_pid);

    /* ── OPÇÃO: -e (executar comando) ── */
    if (strcmp(argv[1], "-e") == 0) {
        if (argc < 4) {
            write(2, "uso: runner -e <user-id> <comando>...\n", 38);
            return 1;
        }

        int  user_id = atoi(argv[2]);
        char command[MAX_CMD];
        if (join_args(command, MAX_CMD, argv, 3, argc) < 0) return 1;

        /* gerar id único para este comando: pid * 100000 + posição */
        static int counter = 0;
        int cmd_id = my_pid * 100 + (++counter);

        /* 1. criar FIFO privado para receber a resposta */
        if (create_fifo(my_fifo) < 0) return 1;

        /* 2. preencher e enviar MSG_REQUEST ao controller */
        Message msg;
        msg.type       = MSG_REQUEST;
        msg.runner_pid = my_pid;
        msg.user_id    = user_id;
        strncpy(msg.command, command, MAX_CMD - 1);
        msg.command[MAX_CMD - 1] = '\0';

        int fd_server = open_fifo_write(SERVER_FIFO);
        if (fd_server < 0) { unlink(my_fifo); return 1; }
        send_message(fd_server, &msg);
        close(fd_server);

        /* 3. notificar utilizador — "submitted" */
        char buf[128];
        int  n;
        n = snprintf(buf, sizeof(buf), "[runner] command %d submitted\n", cmd_id);
        write(1, buf, n);

        /* 4. bloquear à espera do MSG_GO_AHEAD no FIFO privado */
        int fd_private = open_fifo_read(my_fifo);
        if (fd_private < 0) { unlink(my_fifo); return 1; }

        Message res;
        receive_message(fd_private, &res);
        close(fd_private);

        if (res.type != MSG_GO_AHEAD) {
            write(2, "[runner] resposta inesperada do controller\n", 43);
            unlink(my_fifo);
            return 1;
        }

        /* 5. executar o comando */
        n = snprintf(buf, sizeof(buf), "[runner] executing command %d...\n", cmd_id);
        write(1, buf, n);

        execute_command(command);

        n = snprintf(buf, sizeof(buf), "[runner] command %d finished\n", cmd_id);
        write(1, buf, n);

        /* 6. notificar o controller que terminou */
        fd_server = open_fifo_write(SERVER_FIFO);
        if (fd_server < 0) { unlink(my_fifo); return 1; }
        msg.type = MSG_DONE;
        send_message(fd_server, &msg);
        close(fd_server);

        unlink(my_fifo);
        return 0;
    }

    /* ── OPÇÃO: -c (consultar fila) ── */
    if (strcmp(argv[1], "-c") == 0) {
        if (create_fifo(my_fifo) < 0) return 1;

        Message msg;
        msg.type       = MSG_QUERY;
        msg.runner_pid = my_pid;
        msg.user_id    = 0;
        msg.command[0] = '\0';

        int fd_server = open_fifo_write(SERVER_FIFO);
        if (fd_server < 0) { unlink(my_fifo); return 1; }
        send_message(fd_server, &msg);
        close(fd_server);

        int fd_private = open_fifo_read(my_fifo);
        if (fd_private < 0) { unlink(my_fifo); return 1; }

        Message res;
        receive_message(fd_private, &res);
        close(fd_private);

        /* imprimir a resposta do controller */
        write(1, res.command, strlen(res.command));

        unlink(my_fifo);
        return 0;
    }

    /* ── OPÇÃO: -s (shutdown) ── */
    if (strcmp(argv[1], "-s") == 0) {
        if (create_fifo(my_fifo) < 0) return 1;

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

        write(1, "[runner] waiting for controller to shutdown...\n", 47);

        int fd_private = open_fifo_read(my_fifo);
        if (fd_private < 0) { unlink(my_fifo); return 1; }

        Message res;
        receive_message(fd_private, &res);
        unlink_fifo(fd_private, my_fifo);

        write(1, "[runner] controller exited.\n", 28);
        return 0;
    }

    write(2, "opcao invalida. use -e, -c ou -s\n", 33);
    return 1;
}