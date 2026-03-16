#ifndef PROTOCOL_H
#define PROTOCOL_H

#define FIFO_CONTROLLER "tmp/controller_in"
#define FIFO_RUNNER_PREFIX "tmp/runner_"

// message types
#define MSG_REQUEST   1   // runner asking permission to run
#define MSG_DONE      2   // runner saying it finished
#define MSG_GO_AHEAD  3   // controller saying run now

typedef struct {
    int type;           // one of the MSG_ values above
    int runner_pid;     // so controller knows where to reply
    int user_id;
    char command[128];  // the command string e.g. "echo hello"
} Message;

#endif
