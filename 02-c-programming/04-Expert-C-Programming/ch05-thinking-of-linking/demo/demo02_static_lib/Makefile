CC      ?= gcc
AR      ?= ar
CFLAGS  ?= -Wall -Wextra -std=c11 -g

all: demo_static

libadd.a: add.o
	$(AR) rcs $@ add.o

add.o: add.c add.h
	$(CC) $(CFLAGS) -c add.c -o add.o

demo_static: main.o libadd.a
	$(CC) $(CFLAGS) -o $@ main.o -L. -ladd

main.o: main.c add.h
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	rm -f *.o *.a demo_static

.PHONY: all clean
