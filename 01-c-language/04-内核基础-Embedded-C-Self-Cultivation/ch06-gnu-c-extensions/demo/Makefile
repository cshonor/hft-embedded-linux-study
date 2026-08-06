CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g -O0

BINS = demo01_packed_struct demo02_custom_section demo03_weak demo03_weak_override \
       demo04_constructor demo05_flexible_array demo06_log_macro demo07_reg_asm

all: $(BINS)

demo01_packed_struct: demo01_packed_struct.c
demo02_custom_section: demo02_custom_section.c
demo04_constructor: demo04_constructor.c
demo05_flexible_array: demo05_flexible_array.c
demo06_log_macro: demo06_log_macro.c
demo07_reg_asm: demo07_reg_asm.c

demo03_weak: demo03_weak_main.c hw_weak.c
	$(CC) $(CFLAGS) -o $@ demo03_weak_main.c hw_weak.c

demo03_weak_override: demo03_weak_main.c hw_weak.c hw_strong.c
	$(CC) $(CFLAGS) -o $@ demo03_weak_main.c hw_weak.c hw_strong.c

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BINS)

.PHONY: all clean
