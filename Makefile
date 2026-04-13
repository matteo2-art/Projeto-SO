CC     = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS =

CONTROLLER_SRCS = src/controller/main.c \
                  src/controller/handlers.c \
                  src/controller/slots.c \
                  src/common/ipc.c \
                  src/common/scheduler.c \
                  src/common/fcfs.c \
                  src/common/rr.c

RUNNER_SRCS = src/runner/main.c \
              src/runner/parser.c \
              src/common/ipc.c

CONTROLLER_OBJS = $(CONTROLLER_SRCS:src/%.c=obj/%.o)
RUNNER_OBJS     = $(RUNNER_SRCS:src/%.c=obj/%.o)

all: folders bin/controller bin/runner

controller: bin/controller
runner:     bin/runner

folders:
	@mkdir -p bin tmp

bin/controller: $(CONTROLLER_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

bin/runner: $(RUNNER_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj/* tmp/* bin/*