CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
.PHONY: all clean
all:
	$(MAKE) -C demo01_printf
	$(MAKE) -C demo02_strncpy
	$(MAKE) -C demo03_strcpy
	$(MAKE) -C demo04_malloc
	$(MAKE) -C demo05_ctype
	$(MAKE) -C demo06_fgets
	$(MAKE) -C demo07_math
clean:
	$(MAKE) -C demo01_printf clean
	$(MAKE) -C demo02_strncpy clean
	$(MAKE) -C demo03_strcpy clean
	$(MAKE) -C demo04_malloc clean
	$(MAKE) -C demo05_ctype clean
	$(MAKE) -C demo06_fgets clean
	$(MAKE) -C demo07_math clean
