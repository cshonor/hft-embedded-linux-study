CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g -O0

BINS = demo01_array_ptr demo02_double_ptr demo03_struct_offset \
       demo04_func_ptr demo05_volatile demo06_dangling demo07_mmio

all: $(BINS)

demo03_struct_offset: demo03_struct_offset.c
	$(CC) $(CFLAGS) -o $@ $<

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BINS)

.PHONY: all clean
