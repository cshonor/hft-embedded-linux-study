# 8.1 一维数组 · demo（数组 vs 指针 + `= {0}` 语义）
CC      ?= gcc
CFLAGS  ?= -std=c11 -O0 -Wall -Wno-unused -fno-pie -no-pie
CFLAGS2  = -std=c11 -O2 -Wall -Wno-unused -fno-pie -no-pie

BINS = t1_array_vs_pointer t2_zero_init t3_clear_cost t4_param_decay

all: $(BINS)

t1_array_vs_pointer: t1_array_vs_pointer.c
	$(CC) $(CFLAGS) -o $@ $<

t2_zero_init: t2_zero_init.c
	$(CC) $(CFLAGS) -o $@ $<

t3_clear_cost: t3_clear_cost.c
	$(CC) $(CFLAGS2) -o $@ $<

t4_param_decay: t4_param_decay.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	bash run.sh

clean:
	rm -f $(BINS)

.PHONY: all run clean
