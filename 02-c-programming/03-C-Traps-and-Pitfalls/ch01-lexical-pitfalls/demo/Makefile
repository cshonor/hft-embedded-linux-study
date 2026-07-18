CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g

.PHONY: all clean

all:
	$(MAKE) -C demo01_assign_vs_eq
	$(MAKE) -C demo03_char_vs_string
	$(MAKE) -C demo04_octal
	$(MAKE) -C demo05_max_munch

clean:
	$(MAKE) -C demo01_assign_vs_eq clean
	$(MAKE) -C demo03_char_vs_string clean
	$(MAKE) -C demo04_octal clean
	$(MAKE) -C demo05_max_munch clean
