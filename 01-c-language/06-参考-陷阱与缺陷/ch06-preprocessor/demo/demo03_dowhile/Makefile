CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
all: main
main: main.c
	$(CC) $(CFLAGS) -o main main.c
bad:
	$(CC) $(CFLAGS) -o bad -DBUILD_BAD main_bad.c 2>&1 || true
clean:
	rm -f main bad *.o
.PHONY: all clean bad
