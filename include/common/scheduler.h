#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <sys/types.h>   
#include <sys/time.h>

#define MAX_CMD 512

/**
 * @brief SchedPolicy — Define a política de escalonamento
 *
 * @param SCHED_FCFS First-Come-First-Served
 * @param SCHED_RR   Round-Robin
 */
typedef enum {
    SCHED_FCFS = 0,
    SCHED_RR   = 1
} SchedPolicy;

/**
 * @brief Job — Representa uma tarefa (comando) a ser executada
 *
 * @param job_id Identificador único da tarefa
 * @param user_id ID do utilizador que submeteu
 * @param runner_pid PID do processo runner que executará o comando
 * @param command String com o comando bash a executar
 * @param submit_time Timestamp de quando foi submetido (para cálculo de métricas)
 * @param next Ponteiro para a próxima tarefa na fila
 */
typedef struct Job {
    int job_id;
    int user_id;
    pid_t runner_pid;
    char command[MAX_CMD];
    struct timeval submit_time;
    struct Job *next;
} Job;

/**
 * @brief FCFSData — Estrutura de dados para política FCFS
 *
 * @param head Primeira tarefa da fila (próxima a executar)
 * @param tail Última tarefa da fila (última a chegar)
 */
typedef struct {
    Job *head;
    Job *tail;
} FCFSData;

/**
 * @brief UserQueue — Nó da fila circular de Round-Robin
 *
 * @param user_id ID do utilizador
 * @param head Primeira tarefa deste utilizador
 * @param tail Última tarefa deste utilizador
 * @param next Próximo utilizador na lista circular
 */
typedef struct UserQueue {
    int user_id;
    Job *head;
    Job *tail;
    struct UserQueue *next;
} UserQueue;

/**
 * @brief RRData — Estrutura de dados para política Round Robin
 *
 * @param head Primeiro utilizador da fila circular
 * @param current Utilizador atualmente tendo turno
 */
typedef struct {
    UserQueue *head;
    UserQueue *current;
} RRData;

/**
 * @brief Scheduler — Gestor central de tarefas
 *
 * @param policy Política ativa (SCHED_FCFS ou SCHED_RR)
 * @param size Número total de tarefas na(s) fila(s)
 * @param fcfs Dados da fila FCFS (quando aplicável)
 * @param rr Dados da estrutura RR (quando aplicável)
 */
typedef struct {
    SchedPolicy policy;
    int size;
    FCFSData fcfs;
    RRData   rr;
} Scheduler;

/**
 * @brief Aloca e inicializa uma nova tarefa
 *
 * @param job_id Identificador do job
 * @param user_id Identificador do utilizador
 * @param runner_pid PID do runner que submeteu o job
 * @param command Comando a executar (string)
 * @return Job* ponteiro para a nova Job alocada
 */
Job *make_job (int job_id, int user_id, pid_t runner_pid, const char *command);

/**
 * @brief Inicializa o scheduler com a política especificada
 *
 * @param s Ponteiro para o Scheduler a inicializar
 * @param policy Política a usar (SCHED_FCFS ou SCHED_RR)
 * @return void
 */
void scheduler_init (Scheduler *s, SchedPolicy policy);

/**
 * @brief Adiciona uma tarefa à fila (segundo a política ativa)
 *
 * @param s Scheduler onde inserir a tarefa
 * @param job_id Identificador do job
 * @param user_id Identificador do utilizador
 * @param runner_pid PID do runner
 * @param command Comando a executar
 * @return void
 */
void scheduler_add_job (Scheduler *s, int job_id, int user_id, pid_t runner_pid, const char *command);

/**
 * @brief Retira e retorna a próxima tarefa a executar
 *
 * Comportamento depende da política:
 * - FCFS: retira da cabeça da fila
 * - RR: retira do utilizador atual e roda para o próximo
 *
 * @param s Scheduler de onde retirar o job
 * @return Job* ponteiro para o job removido (ou NULL se vazio)
 */
Job *scheduler_next_job(Scheduler *s);

/**
 * @brief Verifica se não há tarefas na fila
 *
 * @param s Scheduler a verificar
 * @return int 1 se vazio, 0 caso contrário
 */
int  scheduler_is_empty(Scheduler *s);

/**
 * @brief Escreve um texto descritivo da fila num buffer (usado por -c)
 *
 * @param s Scheduler a ler
 * @param buf Buffer de destino
 * @param buf_size Tamanho do buffer
 * @return void
 */
void scheduler_list (Scheduler *s, char *buf, int buf_size);

/**
 * @brief Liberta toda a memória alocada pelo scheduler
 *
 * @param s Scheduler a destruir
 * @return void
 */
void scheduler_destroy (Scheduler *s);

#endif