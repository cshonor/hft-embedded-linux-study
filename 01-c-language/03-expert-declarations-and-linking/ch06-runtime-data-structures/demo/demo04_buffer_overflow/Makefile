CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g

all: main

main: main.c
	$(CC) $(CFLAGS) -o main main.c

# 无栈保护时更易观察相邻变量被覆盖（教学环境）
main_noprotect: main.c
	$(CC) $(CFLAGS) -fno-stack-protector -o main_noprotect main.c

clean:
	rm -f main main_noprotect *.o

.PHONY: all clean main_noprotect
