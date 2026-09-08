CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g -fno-stack-protector
all: main
main: main.c
	$(CC) $(CFLAGS) -o main main.c
clean:
	rm -f main *.o
.PHONY: all clean
