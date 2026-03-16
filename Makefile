CC      = gcc
CFLAGS  = -Wall -Wextra -I include

SRCS_CTRL   = src/controller.c
SRCS_RUNNER = src/runner.c

.PHONY: all clean

all: controller runner

controller: $(SRCS_CTRL) include/protocol.h
	$(CC) $(CFLAGS) -o $@ $(SRCS_CTRL)

runner: $(SRCS_RUNNER) include/protocol.h
	$(CC) $(CFLAGS) -o $@ $(SRCS_RUNNER)

clean:
	rm -f controller runner
	rm -f tmp/controller_in tmp/runner_*
