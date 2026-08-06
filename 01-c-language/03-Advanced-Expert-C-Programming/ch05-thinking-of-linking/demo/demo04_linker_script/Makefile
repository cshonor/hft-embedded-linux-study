CC      ?= gcc
LD      ?= ld
CFLAGS  ?= -Wall -Wextra -std=c11 -g -ffreestanding -nostdlib

all: kernel.elf

start.o: start.c
	$(CC) $(CFLAGS) -c start.c -o start.o

kernel.elf: start.o kernel.lds
	$(LD) -T kernel.lds start.o -o kernel.elf
	readelf -S kernel.elf
	nm kernel.elf | grep -E '_stext|_ebss|g_'

clean:
	rm -f start.o kernel.elf

.PHONY: all clean
