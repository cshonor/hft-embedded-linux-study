CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g

.PHONY: all clean

all:
	$(MAKE) -C demo01_const_ptr
	$(MAKE) -C demo02_func_ptr
	$(MAKE) -C demo03_double_ptr
	$(MAKE) -C demo04_void_ptr
	$(MAKE) -C demo05_jagged_vs_2d

clean:
	$(MAKE) -C demo01_const_ptr clean
	$(MAKE) -C demo02_func_ptr clean
	$(MAKE) -C demo03_double_ptr clean
	$(MAKE) -C demo04_void_ptr clean
	$(MAKE) -C demo05_jagged_vs_2d clean
