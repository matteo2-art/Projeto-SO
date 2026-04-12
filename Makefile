CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS =

all: folders controller runner

controller: bin/controller
runner: bin/runner

folders:
	@mkdir -p src include obj bin tmp

bin/controller: obj/controller.o obj/scheduler.o obj/fcfs.o obj/rr.o obj/ipc.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/runner: obj/runner.o obj/parser.o obj/ipc.o
	$(CC) $(LDFLAGS) $^ -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/* tmp/* bin/*