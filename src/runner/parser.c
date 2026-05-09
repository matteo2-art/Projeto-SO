#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "runner/parser.h"

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

        // Copiar a palavra para o buffer exatamente onde ficou-se
        strncpy(buf + offset, argv[i], buf_size - offset - 1);
        offset += token_len;

        // Se esta não for a última palavra, adicionar um espaço a seguir
        if (i < argc - 1) {
            buf[offset++] = ' ';
        }
    }

    buf[offset] = '\0';
    return 0;
}


int parse_command(char *cmd, Segment *segs) {
    int  nseg = 0;          // índice do segmento atual 
    char *token;

    // inicializar o primeiro segmento 
    segs[0].argc     = 0;
    segs[0].file_in  = NULL;
    segs[0].file_out = NULL;
    segs[0].file_err = NULL;

    token = strtok(cmd, " ");
    while (token != NULL && nseg < MAX_SEGMENTS - 1) {

        if (strcmp(token, "|") == 0) {
            // fechar o segmento atual e abrir um novo 
            segs[nseg].args[segs[nseg].argc] = NULL;
            nseg++;
            segs[nseg].argc     = 0;
            segs[nseg].file_in  = NULL;
            segs[nseg].file_out = NULL;
            segs[nseg].file_err = NULL;

        } else if (strcmp(token, ">") == 0) {
            // o próximo token é o ficheiro de stdout 
            token = strtok(NULL, " ");
            if (token) segs[nseg].file_out = token;

        } else if (strcmp(token, "2>") == 0) {
            // o próximo token é o ficheiro de stderr 
            token = strtok(NULL, " ");
            if (token) segs[nseg].file_err = token;

        } else if (strcmp(token, "<") == 0) {
            // o próximo token é o ficheiro de stdin 
            token = strtok(NULL, " ");
            if (token) segs[nseg].file_in = token;

        } else {
            // argumento normal — adicionar ao segmento atual 
            if (segs[nseg].argc < MAX_ARGS - 1)
                segs[nseg].args[segs[nseg].argc++] = token;
        }

        token = strtok(NULL, " ");
    }

    // fechar o último segmento 
    segs[nseg].args[segs[nseg].argc] = NULL;
    return nseg + 1; // devolver o número total de segmentos 
}
