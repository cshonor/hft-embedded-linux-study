CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGETS = ex01_fnptr ex02_macro ex03_union ex04_offsetof ex05_likely

.PHONY: all clean run test

all: $(TARGETS)

ex01_fnptr: ex01_fnptr.c
	$(CC) $(CFLAGS) -o $@ $<

ex02_macro: ex02_macro.c
	$(CC) $(CFLAGS) -o $@ $<

ex03_union: ex03_union.c
	$(CC) $(CFLAGS) -o $@ $<

ex04_offsetof: ex04_offsetof.c
	$(CC) $(CFLAGS) -o $@ $<

ex05_likely: ex05_likely.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	./ex01_fnptr
	./ex02_macro
	./ex03_union
	./ex04_offsetof
	./ex05_likely

test: run

clean:
	rm -f $(TARGETS)
