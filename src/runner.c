#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include "../include/protocol.h"

int main() {
    // 1. build this runner's reply FIFO path using our PID
    pid_t my_pid = getpid();
    char reply_fifo[64];
    snprintf(reply_fifo, sizeof(reply_fifo), "%s%d", FIFO_RUNNER_PREFIX, my_pid);

    // 2. create our reply FIFO
    if (mkfifo(reply_fifo, 0666) == -1) {
        perror("mkfifo reply_fifo");
        return 1;
    }

    // 3. build and send the request to controller
    Message msg;
    msg.type = MSG_REQUEST;
    msg.runner_pid = my_pid;
    msg.user_id = 1;
    strcpy(msg.command, "echo hello");

    int controller_fd = open(FIFO_CONTROLLER, O_WRONLY);
    if (controller_fd == -1) {
        perror("open controller");
        unlink(reply_fifo);
        return 1;
    }
    write(controller_fd, &msg, sizeof(Message));
    close(controller_fd);

    // 4. wait for controller's reply on OUR fifo
    int reply_fd = open(reply_fifo, O_RDONLY);
    if (reply_fd == -1) {
        perror("open reply_fifo");
        unlink(reply_fifo);
        return 1;
    }
    Message reply;
    if (read(reply_fd, &reply, sizeof(Message)) != (ssize_t)sizeof(Message)) {
        perror("read reply");
        close(reply_fd);
        unlink(reply_fifo);
        return 1;
    }
    close(reply_fd);

    // 5. if controller said go ahead, run the command
    if (reply.type == MSG_GO_AHEAD) {
        pid_t child = fork();
        if (child == 0) {
            // inside child: replace ourselves with "echo hello"
            char *args[] = {"echo", "hello", NULL};
            execvp("echo", args);
            perror("execvp");
            exit(1);
        } else {
            // inside parent: wait for child to finish
            wait(NULL);
        }
    }

    // 6. notify controller we are done
    int fd2 = open(FIFO_CONTROLLER, O_WRONLY);
    if (fd2 != -1) {
        msg.type = MSG_DONE;
        write(fd2, &msg, sizeof(Message));
        close(fd2);
    }

    // 7. clean up our reply FIFO
    unlink(reply_fifo);
    return 0;
}
