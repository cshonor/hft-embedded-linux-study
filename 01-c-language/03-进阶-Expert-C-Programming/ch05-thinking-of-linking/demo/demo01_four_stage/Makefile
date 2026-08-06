CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g

.PHONY: all clean inspect

all: main.i main.s main.o main

main.i: main.c
	$(CC) $(CFLAGS) -E main.c -o main.i

main.s: main.i
	$(CC) $(CFLAGS) -S main.i -o main.s

main.o: main.s
	$(CC) $(CFLAGS) -c main.s -o main.o

main: main.o
	$(CC) $(CFLAGS) main.o -o main

inspect: main.o main
	nm main.o
	readelf -S main.o
	readelf -r main.o

clean:
	rm -f main main.i main.s main.o
