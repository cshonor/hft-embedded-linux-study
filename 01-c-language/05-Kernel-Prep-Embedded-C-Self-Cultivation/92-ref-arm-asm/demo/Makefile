# Demo03：C 调用 ARM 汇编 add_asm
# 需要交叉工具链 + qemu（在 x86/WSL 上运行 ARM 二进制）

CROSS_COMPILE ?= arm-linux-gnueabihf-
CC      = $(CROSS_COMPILE)gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g -O0 -static
ASFLAGS ?= -g

OBJS = demo03_main.o add_asm.o

all: demo03

demo03: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

demo03_main.o: demo03_main.c
	$(CC) $(CFLAGS) -c $< -o $@

add_asm.o: add_asm.S
	$(CC) $(ASFLAGS) -c $< -o $@

run: demo03
	qemu-arm ./demo03

clean:
	rm -f *.o demo03

.PHONY: all run clean
