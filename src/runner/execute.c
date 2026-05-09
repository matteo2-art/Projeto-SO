#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "runner/parser.h"

void execute_command(const char *command) {
    if (!command) return;

    // Cópia segura: strtok() modifica a string
    char cmd_copy[MAX_CMD];
    strncpy(cmd_copy, command, MAX_CMD - 1);
    cmd_copy[MAX_CMD - 1] = '\0';

    // Parse em segmentos (separados por |)
    Segment segs[MAX_SEGMENTS];
    int nseg = parse_command(cmd_copy, segs);

    if (nseg == 0 || segs[0].argc == 0) return;

    int pipes[MAX_SEGMENTS - 1][2];
    pid_t pids[MAX_SEGMENTS];

    // Criar processos filhos: um por segmento
    int i;
    for (i = 0; i < nseg; i++) {

        // Criar pipe entre este e o próximo segmento
        if (i < nseg - 1) {
            if (pipe(pipes[i]) < 0) {
                perror("pipe");
                return;
            }
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return;
        }

        if (pids[i] == 0) {
            // stdin do segmento: vem do pipe anterior (exceto primeiro)
            if (i > 0) {
                dup2(pipes[i - 1][0], 0);
                close(pipes[i - 1][0]);
            }

            // stdout do segmento: vai para o próximo pipe (exceto último)
            if (i < nseg - 1) {
                dup2(pipes[i][1], 1);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            // stdout redirecionado (>)
            if (segs[i].file_out) {
                int fd = open(segs[i].file_out,
                              O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open >"); _exit(1); }
                dup2(fd, 1);
                close(fd);
            }

            // stderr redirecionado (2>)
            if (segs[i].file_err) {
                int fd = open(segs[i].file_err,
                              O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open 2>"); _exit(1); }
                dup2(fd, 2);
                close(fd);
            }

            // stdin redirecionado (<)
            if (segs[i].file_in) {
                int fd = open(segs[i].file_in, O_RDONLY);
                if (fd < 0) { perror("open <"); _exit(1); }
                dup2(fd, 0);
                close(fd);
            }

            execvp(segs[i].args[0], segs[i].args);
            perror("execvp");
            _exit(1);
        }

        // Parent: fechar ends não usados do pipe
        if (i > 0) {
            close(pipes[i - 1][0]);
        }

        if (i < nseg - 1) {
            close(pipes[i][1]);
        }
    }

    // Aguardar que todos os filhos terminem
    for (i = 0; i < nseg; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (!WIFEXITED(status))
            perror("waitpid: processo filho terminou de forma anormal");
    }
}
