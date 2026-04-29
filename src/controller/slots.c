#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "common/ipc.h"
#include "common/protocol.h"
#include "common/scheduler.h"
#include "controller/slots.h"


// Abre o FIFO privado do runner e envia MSG_GO_AHEAD
void send_go_ahead(pid_t runner_pid) {
    char    path[64];
    Message msg;
    int     fd;

    sprintf(path, CLIENT_FIFO_TEMPLATE, runner_pid);
    msg.type       = MSG_GO_AHEAD;
    msg.runner_pid = runner_pid;
    msg.user_id    = 0;
    msg.command[0] = '\0';
    fd = open_fifo_write(path);
    if (fd < 0) return;
    send_message(fd, &msg);
    close(fd);
}

// Abre o FIFO privado do runner e envia MSG_RESPONSE com texto dentro do campo command. Usada para responder ao -c e para confirmar o shutdown ao -s.
void send_response(pid_t runner_pid, const char *text) {
    char    path[64];
    Message msg;
    int     fd;

    sprintf(path, CLIENT_FIFO_TEMPLATE, runner_pid);
    msg.type       = MSG_RESPONSE;
    msg.runner_pid = runner_pid;
    msg.user_id    = 0;
    strncpy(msg.command, text, MAX_CMD - 1);
    msg.command[MAX_CMD - 1] = '\0';
    fd = open_fifo_write(path);
    if (fd < 0) return;
    send_message(fd, &msg);
    close(fd);
}

// Retira o próximo job do scheduler, preenche o slot com os dados do job, regista o tempo de início com gettimeofday, e chama send_go_ahead. 
// É o acto de autorizar um runner a executar.
void dispatch_next(Scheduler *s, RunningSlot *slot) {
    Job *job = scheduler_next_job(s);
    if (job == NULL) return;

    slot->active     = 1;
    slot->job_id     = job->job_id;
    slot->user_id    = job->user_id;
    slot->runner_pid = job->runner_pid;
    strncpy(slot->command, job->command, MAX_CMD - 1);
    slot->command[MAX_CMD - 1] = '\0';
    slot->submit_time = job->submit_time;   // ← NOVO: copiar tempo de submissão
    gettimeofday(&slot->start_time, NULL);
    free(job);

    send_go_ahead(slot->runner_pid);
}

// Calcula a duração (end_time - slot->start_time) e escreve uma linha no log.txt. 
// Também regista arrival (submit_time) e first_run (start_time) para cálculo de response time.
// Chamada quando um slot termina, antes de o limpar.
void log_job(int fd_log, RunningSlot *slot, struct timeval *end) {
    char line[MAX_CMD + 256];
    // duração = end - submit_time  (turnaround real)
    long secs  = end->tv_sec  - slot->submit_time.tv_sec;
    long usecs = end->tv_usec - slot->submit_time.tv_usec;
    int  n;

    if (usecs < 0) { secs--; usecs += 1000000; }
    
    // arrival = submit_time em formato decimal (sec.microsec)
    // first_run = start_time em formato decimal (sec.microsec)
    n = snprintf(line, sizeof(line),
        "job=%d user=%d cmd=%s arrival=%ld.%06ld first_run=%ld.%06ld duration=%ld.%06lds\n",
        slot->job_id, slot->user_id, slot->command,
        slot->submit_time.tv_sec, slot->submit_time.tv_usec,
        slot->start_time.tv_sec, slot->start_time.tv_usec,
        secs, usecs);
    write(fd_log, line, n);
}

//  Usada para decidir se há capacidade para despachar um novo job.
int find_free_slot(RunningSlot *slots, int parallel) {
    int i;
    for (i = 0; i < parallel; i++)
        if (!slots[i].active) return i;
    return -1;
}

// o controller só se desliga quando não há nada a correr.
int all_slots_free(RunningSlot *slots, int parallel) {
    int i;
    for (i = 0; i < parallel; i++)
        if (slots[i].active) return 0;
    return 1;
}