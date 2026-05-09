# Projeto SO 

Sistema de orquestração de execução de comandos bash com dois programas: **runner** (cliente que submete comandos) e **controller** (servidor que escalona com FCFS ou Round-Robin).
O controller gerencia permissões e timing via IPC, enquanto o runner executa os comandos.

## Building

O projeto pode ser compilado com:

```bash
$ make
```

Os ficheiros compilados podem ser removidos com:

```bash
$ make clean
```

Para remover também os dados dos testes:

```bash
$ make clean-tests
```

Para compilar apenas o controller ou runner:

```bash
$ make controller
$ make runner
```

## Testes e Gráficos

Para correr os testes automaticamente (cenários homogéneo e heterogéneo) e gerar gráficos:

```bash
$ make teste
```

Isto irá compilar, executar testes e gerar visualizações em `testes/graficos/`.

## Como Usar

### 1. Iniciar o Controller

O controller deve ser iniciado em background, especificando o número de slots paralelos e a política de scheduling:

```bash
# FCFS com 1 slot paralelo
$ ./controller 1 fcfs 

# Round Robin com 2 slots paralelos
$ ./controller 2 rr 

# FCFS com 10 slots paralelos
$ ./controller 10 fcfs 
```

**Parâmetros:**
- Primeiro argumento: número de slots paralelos (1, 2, 10, etc.)
- Segundo argumento: política (`fcfs` ou `rr`)

### 2. Submeter Comandos

Submeter um comando com um utilizador específico:

```bash
$ ./runner -e <user_id> <comando>
```

**Exemplos:**
```bash
$ ./runner -e 1 sleep 5
$ ./runner -e 2 sleep 2
$ ./runner -e 3 "echo Hello"
$ ./runner -e 1 ls -la
```

### 3. Ver Fila e Slots Ativos

```bash
$ ./runner -c
```

Mostra o estado atual:
- **Executing**: comandos em execução nos slots
- **Scheduled**: comandos aguardando na fila

### 4. Desligar o Controller

```bash
$ ./runner -s
```

Aguarda que todos os comandos terminem e depois desliga o controller.
