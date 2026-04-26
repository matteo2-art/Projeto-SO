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


/* ── ESTRUTURAS DE DADOS ─────────────────────────────────────────────────── */

/*
 * Um "segmento" representa um comando simples dentro de uma pipeline.
 * Por exemplo, em "ls -la | grep foo > out.txt", há dois segmentos:
 *   segmento 0: args=["ls", "-la"],        stdin=normal,   stdout=pipe
 *   segmento 1: args=["grep", "foo"],      stdin=pipe,     stdout="out.txt"
 */
#define MAX_SEGMENTS 16  /* máximo de comandos ligados por pipes */
#define MAX_ARGS     64  /* máximo de argumentos por comando     */

typedef struct {
    char *args[MAX_ARGS]; /* os argumentos do comando, ex: ["grep", "foo", NULL] */
    int   argc;           /* quantos argumentos tem                               */
    char *file_in;        /* ficheiro de input  (<),  ou NULL se não houver       */
    char *file_out;       /* ficheiro de output (>),  ou NULL se não houver       */
    char *file_err;       /* ficheiro de stderr (2>), ou NULL se não houver       */
} Segment;


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
     * 3. Criar os pipes necessários.
     *
     * Para N segmentos precisamos de N-1 pipes.
     * Cada pipe é um par de file descriptors: [fd_leitura, fd_escrita]
     *
     * Exemplo com 3 segmentos (A | B | C):
     *   pipes[0] liga A → B   (A escreve em pipes[0][1], B lê de pipes[0][0])
     *   pipes[1] liga B → C   (B escreve em pipes[1][1], C lê de pipes[1][0])
     */
    int pipes[MAX_SEGMENTS - 1][2];
    int i;
    for (i = 0; i < nseg - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }

    /*
     * 4. Lançar um processo filho por segmento.
     *
     * Cada filho:
     *   a) Redireciona o seu stdin/stdout para o pipe correto (se aplicável)
     *   b) Aplica redirecionamentos de ficheiros (>, <, 2>)
     *   c) Chama execvp para se transformar no programa
     *
     * O pai guarda todos os PIDs dos filhos para esperar por eles no fim.
     */
    pid_t pids[MAX_SEGMENTS];

    for (i = 0; i < nseg; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("fork");
            return;
        }

        if (pids[i] == 0) {
            /* ── estamos no FILHO i ── */

            /*
             * a) Ligar o stdin ao pipe do segmento anterior (se não for o primeiro)
             *
             * Exemplo: segmento 1 (B em "A | B | C")
             *   O stdin de B deve vir de pipes[0][0] (lado de leitura do pipe A→B)
             *   dup2(pipes[0][0], 0) faz com que o fd 0 (stdin) passe a ser pipes[0][0]
             *   Depois fechamos pipes[0][0] porque já não precisamos da cópia extra
             */
            if (i > 0) {
                dup2(pipes[i - 1][0], 0);   /* stdin ← pipe anterior (leitura) */
                close(pipes[i - 1][0]);
            }

            /*
             * b) Ligar o stdout ao pipe do segmento seguinte (se não for o último)
             *
             * Exemplo: segmento 0 (A em "A | B | C")
             *   O stdout de A deve ir para pipes[0][1] (lado de escrita do pipe A→B)
             *   dup2(pipes[0][1], 1) faz com que o fd 1 (stdout) passe a ser pipes[0][1]
             */
            if (i < nseg - 1) {
                dup2(pipes[i][1], 1);       /* stdout → pipe seguinte (escrita) */
                close(pipes[i][1]);
            }

            /*
             * c) Fechar TODOS os file descriptors dos pipes que este filho não usa.
             *
             * Isto é crucial — se não fecharmos, o filho seguinte nunca recebe EOF
             * porque há sempre alguém com o lado de escrita aberto.
             */
            int j;
            for (j = 0; j < nseg - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            /*
             * d) Aplicar redirecionamentos de ficheiros (>, <, 2>)
             *
             * Exemplo: "echo hello > out.txt"
             *   Abrimos out.txt para escrita e fazemos dup2(fd, 1)
             *   A partir daqui o stdout deste filho vai para out.txt
             *   Fechamos o fd extra porque o dup2 já criou uma cópia no fd 1
             */
            if (segs[i].file_out) {
                int fd = open(segs[i].file_out,
                              O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open >"); _exit(1); }
                dup2(fd, 1);    /* stdout → ficheiro */
                close(fd);
            }

            if (segs[i].file_err) {
                int fd = open(segs[i].file_err,
                              O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open 2>"); _exit(1); }
                dup2(fd, 2);    /* stderr → ficheiro */
                close(fd);
            }

            if (segs[i].file_in) {
                int fd = open(segs[i].file_in, O_RDONLY);
                if (fd < 0) { perror("open <"); _exit(1); }
                dup2(fd, 0);    /* stdin ← ficheiro */
                close(fd);
            }

            /* e) Transformar este processo no programa pedido */
            execvp(segs[i].args[0], segs[i].args);
            perror("execvp");
            _exit(1);
        }
        /* ── voltámos ao PAI — continuar para o próximo segmento ── */
    }

    /*
     * 5. O pai fecha TODOS os file descriptors dos pipes.
     *
     * O pai nunca lê nem escreve nos pipes — isso é trabalho dos filhos.
     * Se o pai não fechar, os filhos nunca recebem EOF e ficam bloqueados.
     */
    for (i = 0; i < nseg - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    /*
     * 6. Esperar que TODOS os filhos terminem antes de regressar.
     *
     * Usamos waitpid em loop para esperar por cada filho pela ordem em que
     * foi lançado. Só quando todos terminarem é que o runner envia MSG_DONE.
     */
    for (i = 0; i < nseg; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (!WIFEXITED(status))
            perror("waitpid: processo filho terminou de forma anormal");
    }
}