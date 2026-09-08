CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g -O0

all: demo01_arr_ptr demo02_func_ptr demo03_const_typedef demo04_spiral_examples

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f demo0*

.PHONY: all clean
