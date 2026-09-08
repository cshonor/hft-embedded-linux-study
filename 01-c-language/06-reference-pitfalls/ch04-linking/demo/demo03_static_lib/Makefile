CC ?= gcc
AR ?= ar
CFLAGS ?= -Wall -Wextra -std=c11 -g
all: demo
libadd.a: add.o
	$(AR) rcs $@ add.o
add.o: add.c add.h
	$(CC) $(CFLAGS) -c add.c
demo: main.o libadd.a
	$(CC) $(CFLAGS) -o demo main.o -L. -ladd
main.o: main.c add.h
	$(CC) $(CFLAGS) -c main.c
clean:
	rm -f *.o *.a demo
.PHONY: all clean
