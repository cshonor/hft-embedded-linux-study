CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g -O0

all: demo01_switch demo02_sign_unsigned demo03_macro_mul demo04_static_dual

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f demo0*

.PHONY: all clean
