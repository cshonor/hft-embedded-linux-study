CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g

all: demo_weak

demo_weak: weak.c strong.c main.c
	$(CC) $(CFLAGS) -o $@ weak.c strong.c main.c
	nm demo_weak | grep g_val

clean:
	rm -f demo_weak

.PHONY: all clean
