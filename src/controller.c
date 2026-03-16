#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include "../include/protocol.h"

void build_reply_path(char *buf, int pid) {
    // manually build "tmp/runner_XXXX" without printf
    strcpy(buf, FIFO_RUNNER_PREFIX);
    int len = strlen(buf);
    // convert pid to string manually
    char tmp[20];
    int i = 0;
    int n = pid;
    if (n == 0) { tmp[i++] = '0'; }
    while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; }
    // tmp is reversed, fix it
    for (int j = 0; j < i; j++) {
        buf[len + j] = tmp[i - 1 - j];
    }
    buf[len + i] = '\0';
}

int main() {
    mkfifo(FIFO_CONTROLLER, 0666);

    int fd = open(FIFO_CONTROLLER, O_RDWR);

    Message msg;
    while (read(fd, &msg, sizeof(Message)) > 0) {

        if (msg.type == MSG_REQUEST) {
            char reply_fifo[64];
            build_reply_path(reply_fifo, msg.runner_pid);

            int reply_fd = open(reply_fifo, O_WRONLY);
            Message reply;
            reply.type = MSG_GO_AHEAD;
            reply.runner_pid = msg.runner_pid;
            write(reply_fd, &reply, sizeof(Message));
            close(reply_fd);
        }

        if (msg.type == MSG_DONE) {
            // will log here later
            // for now just print to stderr for debugging
            char dbg[] = "[controller] a command finished\n";
            write(2, dbg, sizeof(dbg) - 1);
        }
    }

    close(fd);
    unlink(FIFO_CONTROLLER);
    return 0;
}