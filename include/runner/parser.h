#ifndef PARSER_H
#define PARSER_H

#define MAX_CMD 512
#define MAX_SEGMENTS 16  /* máximo de comandos ligados por pipes */
#define MAX_ARGS     64  /* máximo de argumentos por comando     */

/**
 * @brief Segment — Representa um comando dentro de uma pipeline
 *
 * @param args Array de strings com os argumentos do comando (NULL-terminated)
 * @param argc Número de argumentos em `args`
 * @param file_in Caminho para redirecionamento de stdin (<) ou NULL
 * @param file_out Caminho para redirecionamento de stdout (>) ou NULL
 * @param file_err Caminho para redirecionamento de stderr (2>) ou NULL
 */
typedef struct {
    char *args[MAX_ARGS];
    int   argc;
    char *file_in;
    char *file_out;
    char *file_err;
} Segment;

/**
 * @brief Divide um comando em segmentos separados por pipes.
 *
 * @param cmd Comando a dividir
 * @param segs Array de segmentos para armazenar resultado
 * @return int Número de segmentos
 */
int parse_command(char *cmd, Segment *segs);

/**
 * @brief Junta argumentos num buffer `buf` a partir de `argv[start]` até `argc`.
 *
 * @param buf Buffer de destino
 * @param buf_size Tamanho de `buf`
 * @param argv Array de argumentos
 * @param start Índice inicial em `argv`
 * @param argc Índice final / número de argumentos a juntar
 * @return int Comprimento escrito no buffer (número de bytes)
 */
int join_args(char *buf, int buf_size, char *argv[], int start, int argc);

#endif