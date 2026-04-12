#ifndef PARSER_H
#define PARSER_H

#define MAX_CMD 512

void execute_command(const char *command);
int join_args(char *buf, int buf_size, char *argv[], int start, int argc);

#endif