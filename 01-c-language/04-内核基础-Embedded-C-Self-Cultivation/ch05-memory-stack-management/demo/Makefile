CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g -O0

BINS = demo01_memory_zone demo02_stack_frame demo03_stack_overflow demo04_heap_leak demo05_static

all: $(BINS)

demo01_memory_zone: demo01_memory_zone.c
demo02_stack_frame: demo02_stack_frame.c
demo03_stack_overflow: demo03_stack_overflow.c
demo04_heap_leak: demo04_heap_leak.c
demo05_static: demo05_static.c

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BINS)

.PHONY: all clean
