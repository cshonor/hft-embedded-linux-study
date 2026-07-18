CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g -O0

all: demo01_endian

demo01_endian: demo01_endian.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f demo01_endian

.PHONY: all clean
