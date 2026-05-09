#ifndef EXECUTE_H
#define EXECUTE_H

/**
 * @brief Interpreta e executa a string `command` (fork + exec).
 *
 * Suporta pipes (|), redirecionamento de stdin (<), stdout (>), e stderr (2>).
 * Cria um processo filho por segmento de pipeline.
 *
 * @param command Comando a executar (ex: "cat file.txt | grep pattern > output.txt")
 * @return void
 */
void execute_command(const char *command);

#endif
