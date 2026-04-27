#ifndef PARSER_H
#define PARSER_H

#define MAX_CMD 512

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


void execute_command(const char *command);
int join_args(char *buf, int buf_size, char *argv[], int start, int argc);

#endif