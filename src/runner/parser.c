#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "runner/parser.h"

/* ── FUNÇÕES AUXILIARES (HELPERS) ──────────────────────────────────────── */

/* * Esta função volta a colar o array argv numa única string.
 * Exemplo: argv[3]="echo", argv[4]="hello"  --> buf="echo hello"
 */
int join_args(char *buf, int buf_size, char *argv[], int start, int argc) {
    buf[0] = '\0'; // Começar com uma string vazia
    int offset = 0; // Controla onde estamos a escrever atualmente no buffer

    // Percorrer os argumentos começando pelo nome do comando
    for (int i = start; i < argc; i++) {
        int token_len = strlen(argv[i]);

        // Verificação de segurança
        // +2 serve para o espaço que adicionamos, e para o terminador \0 no final
        if (offset + token_len + 2 > buf_size) {
            write(2, "erro: comando demasiado longo\n", 30); 
            return -1;
        }

        // Copiar a palavra para o buffer exatamente onde ficámos
        strncpy(buf + offset, argv[i], buf_size - offset - 1);
        offset += token_len;

        // Se esta não for a última palavra, adicionar um espaço a seguir
        if (i < argc - 1) {
            buf[offset++] = ' ';
        }
    }

    // Selar a string final com um terminador nulo
    buf[offset] = '\0';
    return 0;
}


/* ── PARSER: converte a string do comando em segmentos ──────────────────── */

/*
 * Recebe a string do comando (ex: "ls | wc -l > out.txt")
 * e preenche o array de Segment, devolvendo o número de segmentos.
 *
 * Algoritmo:
 *   1. Cortar por espaços com strtok
 *   2. Se o token for "|"  → fechar segmento atual, começar um novo
 *   3. Se o token for ">"  → o próximo token é o ficheiro de stdout
 *   4. Se o token for "2>" → o próximo token é o ficheiro de stderr
 *   5. Se o token for "<"  → o próximo token é o ficheiro de stdin
 *   6. Caso contrário      → é um argumento normal do comando atual
 */
static int parse_command(char *cmd, Segment *segs) {
    int  nseg = 0;          /* índice do segmento atual */
    char *token;

    /* inicializar o primeiro segmento */
    segs[0].argc     = 0;
    segs[0].file_in  = NULL;
    segs[0].file_out = NULL;
    segs[0].file_err = NULL;

    token = strtok(cmd, " ");
    while (token != NULL && nseg < MAX_SEGMENTS - 1) {

        if (strcmp(token, "|") == 0) {
            /* fechar o segmento atual e abrir um novo */
            segs[nseg].args[segs[nseg].argc] = NULL;
            nseg++;
            segs[nseg].argc     = 0;
            segs[nseg].file_in  = NULL;
            segs[nseg].file_out = NULL;
            segs[nseg].file_err = NULL;

        } else if (strcmp(token, ">") == 0) {
            /* o próximo token é o ficheiro de stdout */
            token = strtok(NULL, " ");
            if (token) segs[nseg].file_out = token;

        } else if (strcmp(token, "2>") == 0) {
            /* o próximo token é o ficheiro de stderr */
            token = strtok(NULL, " ");
            if (token) segs[nseg].file_err = token;

        } else if (strcmp(token, "<") == 0) {
            /* o próximo token é o ficheiro de stdin */
            token = strtok(NULL, " ");
            if (token) segs[nseg].file_in = token;

        } else {
            /* argumento normal — adicionar ao segmento atual */
            if (segs[nseg].argc < MAX_ARGS - 1)
                segs[nseg].args[segs[nseg].argc++] = token;
        }

        token = strtok(NULL, " ");
    }

    /* fechar o último segmento */
    segs[nseg].args[segs[nseg].argc] = NULL;
    return nseg + 1; /* devolver o número total de segmentos */
}


/* ── MOTOR DE EXECUÇÃO ───────────────────────────────────────────────────── */

void execute_command(const char *command) {
    if (!command) return;

    /* 1. Cópia segura da string — o strtok vai modificá-la */
    char cmd_copy[MAX_CMD];
    strncpy(cmd_copy, command, MAX_CMD - 1);
    cmd_copy[MAX_CMD - 1] = '\0';

    /* 2. Fazer o parse — dividir em segmentos */
    Segment segs[MAX_SEGMENTS];
    int nseg = parse_command(cmd_copy, segs);

    if (nseg == 0 || segs[0].argc == 0) return;

    /*
     * 3. Array para guardar os pipes — mas agora NÃO os criamos todos aqui.
     * Cada pipe[i] é criado imediatamente antes do fork do segmento i,
     * e fechado no pai logo após o fork do segmento i+1.
     * Em qualquer momento, o pai tem no máximo dois pipes abertos:
     * o do segmento anterior (pipe[i-1]) e o atual (pipe[i]).
     */
    int pipes[MAX_SEGMENTS - 1][2];

    pid_t pids[MAX_SEGMENTS];

    int i;
    for (i = 0; i < nseg; i++) {

        /*
         * Criar pipe[i] agora, imediatamente antes do fork do segmento i.
         * O último segmento não precisa de pipe de saída.
         */
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
            /* ── estamos no FILHO i ── */

            /*
             * a) Ligar o stdin ao pipe do segmento anterior (se não for o primeiro).
             * pipe[i-1][0] é o lado de leitura do pipe anterior.
             * Depois de dup2, fechamos ambos os lados — o pai já fechou pipe[i-1][1]
             * no ciclo anterior, mas o filho recebeu cópias dos fds ao nascer.
             */
            if (i > 0) {
                dup2(pipes[i - 1][0], 0);
                close(pipes[i - 1][0]);
            }

            /*
             * b) Ligar o stdout ao pipe atual (se não for o último).
             * pipe[i][1] é o lado de escrita do pipe que acabámos de criar.
             * Depois de dup2, fechamos ambos os lados.
             */
            if (i < nseg - 1) {
                dup2(pipes[i][1], 1);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            /*
             * Nota: já não precisamos do grande loop a fechar todos os pipes,
             * porque em qualquer momento só estão abertos pipe[i-1] e pipe[i],
             * e ambos foram tratados nos passos a) e b) acima.
             */

            /* c) Aplicar redirecionamentos de ficheiros (>, <, 2>) */
            if (segs[i].file_out) {
                int fd = open(segs[i].file_out,
                              O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open >"); _exit(1); }
                dup2(fd, 1);
                close(fd);
            }

            if (segs[i].file_err) {
                int fd = open(segs[i].file_err,
                              O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open 2>"); _exit(1); }
                dup2(fd, 2);
                close(fd);
            }

            if (segs[i].file_in) {
                int fd = open(segs[i].file_in, O_RDONLY);
                if (fd < 0) { perror("open <"); _exit(1); }
                dup2(fd, 0);
                close(fd);
            }

            /* d) Transformar este processo no programa pedido */
            execvp(segs[i].args[0], segs[i].args);
            perror("execvp");
            _exit(1);
        }

        /* ── voltámos ao PAI ── */

        /*
         * Fechar pipe[i-1] completamente — o filho i já o herdou e
         * nenhum outro segmento vai precisar dele. Fazemos isto logo aqui,
         * não no final, para nunca termos mais de dois pipes abertos ao mesmo tempo.
         */
        if (i > 0) {
            close(pipes[i - 1][0]);
        }

        /*
         * Fechar a ponta de escrita de pipe[i] no pai.
         * O pai nunca escreve nos pipes — só os filhos é que o fazem.
         * Se não fecharmos, o filho seguinte nunca recebe EOF.
         */
        if (i < nseg - 1) {
            close(pipes[i][1]);
        }
    }

    /*
     * 5. Esperar que todos os filhos terminem.
     * Nota: já não há loop a fechar pipes aqui — foram todos fechados
     * incrementalmente dentro do ciclo de forks acima.
     */
    for (i = 0; i < nseg; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (!WIFEXITED(status))
            perror("waitpid: processo filho terminou de forma anormal");
    }
}