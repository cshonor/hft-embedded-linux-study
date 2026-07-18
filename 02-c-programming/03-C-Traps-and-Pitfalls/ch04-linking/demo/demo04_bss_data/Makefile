CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
all: main
main: main.c
	$(CC) $(CFLAGS) -o main main.c
	-nm main | grep -E 'g_bss|g_data' || true
clean:
	rm -f main *.o
.PHONY: all clean
