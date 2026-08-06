CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

calc: main.o stack.o getop.o
	$(CC) $(CFLAGS) -o calc main.o stack.o getop.o

main.o: main.c calc.h
	$(CC) $(CFLAGS) -c main.c

stack.o: stack.c stack.h calc.h
	$(CC) $(CFLAGS) -c stack.c

getop.o: getop.c calc.h
	$(CC) $(CFLAGS) -c getop.c

clean:
	rm -f *.o calc

.PHONY: clean
