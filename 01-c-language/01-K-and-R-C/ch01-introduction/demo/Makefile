CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
.PHONY: all clean
all:
	$(MAKE) -C demo_argv
	$(MAKE) -C demo_argv_usage
clean:
	$(MAKE) -C demo_argv clean
	$(MAKE) -C demo_argv_usage clean
