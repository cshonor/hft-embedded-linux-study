CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
all: use_wrong use_correct
use_wrong: use_wrong.c arr_def.c
	$(CC) $(CFLAGS) -o use_wrong use_wrong.c arr_def.c
use_correct: use_correct.c arr_def.c
	$(CC) $(CFLAGS) -o use_correct use_correct.c arr_def.c
clean:
	rm -f use_wrong use_correct *.o
.PHONY: all clean
