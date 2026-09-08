CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
.PHONY: all clean
all:
	$(MAKE) -C demo01_extern
	$(MAKE) -C demo02_extern_type
	$(MAKE) -C demo03_static_lib
	$(MAKE) -C demo04_bss_data
	$(MAKE) -C demo05_undef
clean:
	$(MAKE) -C demo01_extern clean
	$(MAKE) -C demo02_extern_type clean
	$(MAKE) -C demo03_static_lib clean
	$(MAKE) -C demo04_bss_data clean
	$(MAKE) -C demo05_undef clean
