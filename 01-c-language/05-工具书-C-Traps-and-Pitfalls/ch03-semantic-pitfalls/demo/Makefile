CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
.PHONY: all clean
all:
	$(MAKE) -C demo01_array_ptr
	$(MAKE) -C demo02_signed_unsigned
	$(MAKE) -C demo03_unsigned_loop
	$(MAKE) -C demo04_arg_order
	$(MAKE) -C demo05_bounds
	$(MAKE) -C demo06_ro_string
clean:
	$(MAKE) -C demo01_array_ptr clean
	$(MAKE) -C demo02_signed_unsigned clean
	$(MAKE) -C demo03_unsigned_loop clean
	$(MAKE) -C demo04_arg_order clean
	$(MAKE) -C demo05_bounds clean
	$(MAKE) -C demo06_ro_string clean
