CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g

.PHONY: all clean

all:
	$(MAKE) -C demo01_2d_pass
	$(MAKE) -C demo02_string_arrays
	$(MAKE) -C demo03_flex_vs_ptr
	$(MAKE) -C demo04_ptr_arith_2d

clean:
	$(MAKE) -C demo01_2d_pass clean
	$(MAKE) -C demo02_string_arrays clean
	$(MAKE) -C demo03_flex_vs_ptr clean
	$(MAKE) -C demo04_ptr_arith_2d clean
