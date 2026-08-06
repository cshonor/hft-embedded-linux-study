CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
all: demo
demo: main.c helper.c
	$(CC) $(CFLAGS) -o demo main.c helper.c
link_fail:
	$(CC) $(CFLAGS) -o demo_fail main.c || true
clean:
	rm -f demo demo_fail *.o
.PHONY: all clean link_fail
