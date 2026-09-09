CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g -pthread

all: main

main: main.c
	$(CC) $(CFLAGS) -o main main.c -pthread

clean:
	rm -f main *.o

.PHONY: all clean
