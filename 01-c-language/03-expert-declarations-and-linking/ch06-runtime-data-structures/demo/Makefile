CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g

.PHONY: all clean

all:
	$(MAKE) -C demo01_memory_layout
	$(MAKE) -C demo02_static_local
	$(MAKE) -C demo03_stack_frame
	$(MAKE) -C demo04_buffer_overflow

clean:
	$(MAKE) -C demo01_memory_layout clean
	$(MAKE) -C demo02_static_local clean
	$(MAKE) -C demo03_stack_frame clean
	$(MAKE) -C demo04_buffer_overflow clean
