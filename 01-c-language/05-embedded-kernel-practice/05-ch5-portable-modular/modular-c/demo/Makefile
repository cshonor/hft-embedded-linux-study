CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g -O0

.PHONY: all clean mod_uart common demo03

all: common demo01 demo02 demo04 demo05 demo06

common:
	$(MAKE) -C common

demo01:
	$(MAKE) -C demo01_minimal

demo02:
	$(MAKE) -C demo02_make

demo04:
	$(MAKE) -C demo04_weak demo04_weak demo04_weak_board

demo05:
	$(MAKE) -C demo05_log_err

demo06:
	$(MAKE) -C demo06_callback

mod_uart:
	$(MAKE) -C demo02_make mod_uart

demo03:
	mkdir -p demo03_cmake/build
	cd demo03_cmake/build && cmake .. && cmake --build .

clean:
	$(MAKE) -C common clean
	$(MAKE) -C demo01_minimal clean
	$(MAKE) -C demo02_make clean
	$(MAKE) -C demo04_weak clean
	$(MAKE) -C demo05_log_err clean
	$(MAKE) -C demo06_callback clean
	rm -rf demo03_cmake/build

.PHONY: all clean mod_uart common demo03
