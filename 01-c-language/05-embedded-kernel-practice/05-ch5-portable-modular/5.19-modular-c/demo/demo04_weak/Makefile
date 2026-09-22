CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g -O0 -I../common

demo04_weak: app/main.c platform/platform_weak.c
	$(CC) $(CFLAGS) -o $@ app/main.c platform/platform_weak.c

demo04_weak_board: app/main.c platform/platform_weak.c hw/hw_board.c
	$(CC) $(CFLAGS) -o $@ app/main.c platform/platform_weak.c hw/hw_board.c

clean:
	rm -f demo04_weak demo04_weak_board

.PHONY: clean
