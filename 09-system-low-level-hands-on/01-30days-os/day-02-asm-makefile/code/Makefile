# Day 2 · 启动区 IPL + Makefile
# 用法：
#   make ipl   → ipl.bin（512 B）
#   make img   → helloos.img（1.44 MB，ipl.bin 写入偏移 0）
#   make run   → QEMU 启动（需安装 qemu-system-i386）
# Windows 拼盘：.\build-img.ps1

NASM = nasm
NASMFLAGS = -f bin
QEMU = qemu-system-i386

IMG_SIZE = 1474560
IMG_SECTORS = 2880

.PHONY: all clean ipl img run

all: img

ipl: helloos.asm
	$(NASM) $(NASMFLAGS) $< -o ipl.bin

img: ipl.bin
	dd if=/dev/zero of=helloos.img bs=512 count=$(IMG_SECTORS) 2>/dev/null
	dd if=ipl.bin of=helloos.img conv=notrunc

run: helloos.img
	$(QEMU) -fda helloos.img -boot a

clean:
	rm -f ipl.bin helloos.img os-image.bin
