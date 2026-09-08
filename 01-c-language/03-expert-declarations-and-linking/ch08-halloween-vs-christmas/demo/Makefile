CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g

.PHONY: all clean

all:
	$(MAKE) -C demo01_christmas
	$(MAKE) -C demo02_halloween
	$(MAKE) -C demo03_bitwise
	$(MAKE) -C demo04_short_circuit
	$(MAKE) -C demo05_comma_func
	$(MAKE) -C demo06_octal_pun

clean:
	$(MAKE) -C demo01_christmas clean
	$(MAKE) -C demo02_halloween clean
	$(MAKE) -C demo03_bitwise clean
	$(MAKE) -C demo04_short_circuit clean
	$(MAKE) -C demo05_comma_func clean
	$(MAKE) -C demo06_octal_pun clean
