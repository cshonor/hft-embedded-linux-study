CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
all: main
main: main.c def.c common.h
	$(CC) $(CFLAGS) -o main main.c def.c
clean:
	rm -f main *.o
.PHONY: all clean
