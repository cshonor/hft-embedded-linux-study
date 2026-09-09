CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c89 -g -O0

all: demo01_kr_vs_ansi demo02_macro_trap

demo01_kr_vs_ansi: demo01_kr_vs_ansi.c
demo02_macro_trap: demo02_macro_trap.c

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f demo01_kr_vs_ansi demo02_macro_trap

.PHONY: all clean
