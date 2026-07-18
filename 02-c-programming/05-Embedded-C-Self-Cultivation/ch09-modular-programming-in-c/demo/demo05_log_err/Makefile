CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g -O0 -I../common

demo05_log_err: app/main.c ../common/err.c
	$(CC) $(CFLAGS) -o $@ app/main.c ../common/err.c

clean:
	rm -f demo05_log_err

.PHONY: clean
