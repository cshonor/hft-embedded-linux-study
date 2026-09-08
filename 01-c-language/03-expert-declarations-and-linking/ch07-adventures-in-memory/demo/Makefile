CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g

.PHONY: all clean

all:
	$(MAKE) -C demo01_struct_align
	$(MAKE) -C demo02_stack_dangle
	$(MAKE) -C demo03_const_rodata
	$(MAKE) -C demo04_false_sharing

clean:
	$(MAKE) -C demo01_struct_align clean
	$(MAKE) -C demo02_stack_dangle clean
	$(MAKE) -C demo03_const_rodata clean
	$(MAKE) -C demo04_false_sharing clean
