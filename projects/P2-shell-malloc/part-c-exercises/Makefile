CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGETS = ex01_fnptr ex02_macro ex03_union

.PHONY: all clean run

all: $(TARGETS)

ex01_fnptr: ex01_fnptr.c
	$(CC) $(CFLAGS) -o $@ $<

ex02_macro: ex02_macro.c
	$(CC) $(CFLAGS) -o $@ $<

ex03_union: ex03_union.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	./ex01_fnptr
	./ex02_macro
	./ex03_union

clean:
	rm -f $(TARGETS)
