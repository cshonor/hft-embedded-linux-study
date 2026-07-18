CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
all: main
main: main.c
	$(CC) $(CFLAGS) -o main main.c
double_free: double_free.c
	$(CC) $(CFLAGS) -o double_free double_free.c || true
clean:
	rm -f main double_free *.o
.PHONY: all clean double_free
