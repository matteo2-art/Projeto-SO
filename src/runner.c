#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include "../include/protocol.h"

void build_reply_path(char *buf, int pid) {
    strcpy(buf, FIFO_RUNNER_PREFIX);
    int len = strlen(buf);
    char tmp[20];
    int i = 0;
    int n = pid;
    if (n == 0) { tmp[i++] = '0'; }
    while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; }
    for (int j = 0; j < i; j++) {
        buf[len + j] = tmp[i - 1 - j];
    }
    buf[len + i] = '\0';
}

int main() {
    int pid = getpid();
    char reply_fifo[64];
    build_reply_path(reply_fifo, pid);

    // create our personal reply FIFO
    mkfifo(reply_fifo, 0666);

    // send request to controller
    Message msg;
    msg.type = MSG_REQUEST;
    msg.runner_pid = pid;
    msg.user_id = 1;
    strcpy(msg.command, "echo hello");

    int controller_fd = open(FIFO_CONTROLLER, O_WRONLY);
    write(controller_fd, &msg, sizeof(Message));
    close(controller_fd);

    // write to stdout: "submitted"
    char s1[] = "[runner] command submitted\n";
    write(1, s1, sizeof(s1) - 1);

    // wait for controller reply
    int reply_fd = open(reply_fifo, O_RDONLY);
    Message reply;
    read(reply_fd, &reply, sizeof(Message));
    close(reply_fd);

    if (reply.type == MSG_GO_AHEAD) {
        char s2[] = "[runner] executing command...\n";
        write(1, s2, sizeof(s2) - 1);

        pid_t child = fork();
        if (child == 0) {
            // child: run "echo hello"
            char *args[] = {"echo", "hello", NULL};
            execvp("echo", args);
        } else {
            // parent: wait for child
            wait(NULL);
        }
    }

    // notify controller we are done
    int fd2 = open(FIFO_CONTROLLER, O_WRONLY);
    msg.type = MSG_DONE;
    write(fd2, &msg, sizeof(Message));
    close(fd2);

    char s3[] = "[runner] command finished\n";
    write(1, s3, sizeof(s3) - 1);

    unlink(reply_fifo);
    return 0;
}