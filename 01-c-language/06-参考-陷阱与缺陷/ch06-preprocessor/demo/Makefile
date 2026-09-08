CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
.PHONY: all clean
all:
	$(MAKE) -C demo01_parens
	$(MAKE) -C demo02_side_effect
	$(MAKE) -C demo03_dowhile
	$(MAKE) -C demo04_stringify
	$(MAKE) -C demo05_token_paste
	$(MAKE) -C demo06_cond_compile
	$(MAKE) -C demo07_max
clean:
	$(MAKE) -C demo01_parens clean
	$(MAKE) -C demo02_side_effect clean
	$(MAKE) -C demo03_dowhile clean
	$(MAKE) -C demo04_stringify clean
	$(MAKE) -C demo05_token_paste clean
	$(MAKE) -C demo06_cond_compile clean
	$(MAKE) -C demo07_max clean
