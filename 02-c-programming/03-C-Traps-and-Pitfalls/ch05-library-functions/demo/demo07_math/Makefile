CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
LDFLAGS ?= -lm
all: main
main: main.c
	$(CC) $(CFLAGS) -o main main.c $(LDFLAGS)
clean:
	rm -f main *.o
.PHONY: all clean
