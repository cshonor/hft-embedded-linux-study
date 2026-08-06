CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
debug ?= 0
all: main
main: main.c demo_cfg.h
	$(CC) $(CFLAGS) -DDEBUG=$(debug) -o main main.c
clean:
	rm -f main *.o
.PHONY: all clean
