#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "../include/ipc.h"

int create_fifo(const char *path) {
    if (mkfifo(path, 0666) < 0) {
        perror("mkfifo");
        return -1;
    }
    return 0;
}

int open_fifo_write(const char *path) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) perror("open write");
    return fd;
}

int open_fifo_read(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) perror("open read");
    return fd;
}

ssize_t send_message(int fd, Message *msg) {
    return write(fd, msg, sizeof(Message));
}

ssize_t receive_message(int fd, Message *msg) {
    return read(fd, msg, sizeof(Message));
}

void unlink_fifo(int fd, const char *path) {
    if (fd >= 0) close(fd);
    unlink(path);
}