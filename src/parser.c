#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "../include/parser.h"

/* ── MOTOR DE EXECUÇÃO (O cérebro do processo filho) ──────────────────────────────────────── */

void execute_command(const char *command) {
    if (!command) return;

    // 1. Criar uma cópia segura da string do comando
    char cmd_copy[MAX_CMD];
    strncpy(cmd_copy, command, MAX_CMD - 1);
    cmd_copy[MAX_CMD - 1] = '\0'; // Garantir sempre que a string termina corretamente

    char *args[64]; // Array para guardar as palavras separadas (ex: ["echo", "hello", NULL])
    int argc = 0;

    // 2. Cortar a string em pedaços usando espaços como delimitador
    char *token = strtok(cmd_copy, " ");
    while (token != NULL && argc < 63) {
        args[argc++] = token; // Guardar a palavra no nosso array
        token = strtok(NULL, " "); // Obter a próxima palavra
    }
    args[argc] = NULL; // O execvp exige que o último elemento do array seja NULL!

    if (argc == 0) return; // Se o utilizador apenas digitou espaços, não fazer nada

    // 3. Clonar o processo! 
    // A partir desta linha, DOIS programas idênticos estão a correr simultaneamente.
    pid_t pid = fork();

    if (pid < 0) {
        // pid é negativo: O SO falhou ao criar um clone (talvez por falta de memória)
        perror("fork");
        return;
    }

    if (pid == 0) {
        // pid é 0: Estamos dentro do processo FILHO.
        // O execvp substitui completamente este processo filho pelo novo programa (ex: "ls" ou "echo").
        execvp(args[0], args);
        
        // Se o execvp for bem-sucedido, o filho é destruído e substituído. 
        // Portanto, estas próximas duas linhas APENAS correm se o execvp falhar (ex: erro de digitação como "echho").
        perror("execvp");
        _exit(1); 
    } else {
        // pid é > 0: Estamos no processo PAI. 'pid' é o ID do nosso filho.
        int status;
        
        // Colocar o pai a dormir (em espera) até que o processo filho específico termine a execução
        waitpid(pid, &status, 0);
        
        // WIFEXITED verifica se o filho crashou ou terminou normalmente
        if (!WIFEXITED(status)) perror("waitpid: processo filho terminou de forma anormal");
    }
}

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