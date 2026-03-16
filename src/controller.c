#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../include/protocol.h"

int main() {
    // 1. create the controller's inbox FIFO
    if (mkfifo(FIFO_CONTROLLER, 0666) == -1) {
        perror("mkfifo controller");
        return 1;
    }

    // 2. open it for reading (blocks until a runner connects)
    int fd = open(FIFO_CONTROLLER, O_RDONLY);
    if (fd == -1) {
        perror("open controller");
        unlink(FIFO_CONTROLLER);
        return 1;
    }

    // 3. read messages in a loop
    Message msg;
    while (read(fd, &msg, sizeof(Message)) > 0) {

        if (msg.type == MSG_REQUEST) {
            // build the reply FIFO path for this runner
            char reply_fifo[64];
            snprintf(reply_fifo, sizeof(reply_fifo), "%s%d",
                     FIFO_RUNNER_PREFIX, msg.runner_pid);

            // send GO_AHEAD back to that runner
            int reply_fd = open(reply_fifo, O_WRONLY);
            if (reply_fd == -1) {
                perror("open reply_fifo");
                continue;
            }
            Message reply;
            reply.type = MSG_GO_AHEAD;
            if (write(reply_fd, &reply, sizeof(Message)) == -1)
                perror("write reply");
            close(reply_fd);
        }

        if (msg.type == MSG_DONE) {
            // command finished — log it, update state, etc.
        }
    }

    close(fd);
    unlink(FIFO_CONTROLLER);
    return 0;
}
