CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g

.PHONY: all clean

all:
	$(MAKE) -C demo01_bitwise
	$(MAKE) -C demo02_dangling_else
	$(MAKE) -C demo03_func_ptr
	$(MAKE) -C demo04_missing_braces
	$(MAKE) -C demo05_comma

clean:
	$(MAKE) -C demo01_bitwise clean
	$(MAKE) -C demo02_dangling_else clean
	$(MAKE) -C demo03_func_ptr clean
	$(MAKE) -C demo04_missing_braces clean
	$(MAKE) -C demo05_comma clean
