CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
.PHONY: all clean
all:
	$(MAKE) -C demo01_stdint
	$(MAKE) -C demo02_char_sign
	$(MAKE) -C demo03_endian
	$(MAKE) -C demo04_padding
	$(MAKE) -C demo05_uintptr
	$(MAKE) -C demo06_shift
	$(MAKE) -C demo07_platform
clean:
	$(MAKE) -C demo01_stdint clean
	$(MAKE) -C demo02_char_sign clean
	$(MAKE) -C demo03_endian clean
	$(MAKE) -C demo04_padding clean
	$(MAKE) -C demo05_uintptr clean
	$(MAKE) -C demo06_shift clean
	$(MAKE) -C demo07_platform clean
