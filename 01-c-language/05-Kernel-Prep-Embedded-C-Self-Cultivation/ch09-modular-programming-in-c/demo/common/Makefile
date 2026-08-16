CC      ?= gcc
AR      ?= ar
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g -O0

libcommon.a: err.c
	$(CC) $(CFLAGS) -c err.c -o err.o
	$(AR) cr $@ err.o

clean:
	rm -f libcommon.a err.o

.PHONY: clean
