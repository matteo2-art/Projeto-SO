#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common/scheduler.h"
#include "common/ipc.h"
#include "common/protocol.h"
#include "controller/slots.h"
#include "controller/handlers.h"

int main(int argc, char *argv[]) {
    Scheduler   s;
    SchedPolicy policy;
    RunningSlot slots[MAX_PARALLEL];
    Message     msg;
    int         fd_server, fd_log;
    int         parallel;
    int         next_job_id  = 1;
    int         shutdown_req = 0;
    int         should_exit  = 0;
    pid_t       shutdown_pid = -1;
    int         i;

    if (argc < 3) {
        write(2, "uso: controller <parallel-commands> <sched-policy>\n", 51);
        return 1;
    }

    parallel = atoi(argv[1]);
    if (parallel <= 0 || parallel > MAX_PARALLEL) {
        write(2, "erro: parallel-commands invalido\n", 32);
        return 1;
    }

    if      (strcmp(argv[2], "fcfs") == 0) policy = SCHED_FCFS;
    else if (strcmp(argv[2], "rr")   == 0) policy = SCHED_RR;
    else {
        write(2, "erro: politica invalida (use 'fcfs' ou 'rr')\n", 45);
        return 1;
    }

    scheduler_init(&s, policy);
    for (i = 0; i < MAX_PARALLEL; i++) slots[i].active = 0;

    unlink(SERVER_FIFO);
    if (create_fifo(SERVER_FIFO) < 0) return 1;

    fd_server = open(SERVER_FIFO, O_RDWR);
    if (fd_server < 0) { perror("open server fifo"); return 1; }

    fd_log = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd_log < 0) { perror("open log"); return 1; }

    write(1, "[controller] a correr\n", 22);

    while (!should_exit) {
        if (receive_message(fd_server, &msg) <= 0) continue;

        if      (msg.type == MSG_REQUEST)
            handle_request (&msg, &s, slots, parallel, &next_job_id);
        else if (msg.type == MSG_DONE)
            handle_done    (&msg, &s, slots, parallel, fd_log, shutdown_req, shutdown_pid, &should_exit);
        else if (msg.type == MSG_QUERY)
            handle_query   (&msg, &s, slots, parallel);
        else if (msg.type == MSG_SHUTDOWN)
            handle_shutdown(&msg, slots, parallel, &s, &shutdown_pid, &shutdown_req, &should_exit);
    }

    scheduler_destroy(&s);
    close(fd_server);
    close(fd_log);
    unlink(SERVER_FIFO);
    write(1, "[controller] desligado\n", 23);
    return 0;
}