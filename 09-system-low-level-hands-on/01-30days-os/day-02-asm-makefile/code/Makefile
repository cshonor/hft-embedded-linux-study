# Day 2 · 第一天极简 Makefile（配方行首必须是 Tab）
# 说明：notes/section-2.4-Makefile-入门.md
# 源码名：boot.asm 或 helloos.nas（改下一行依赖即可）

NASM = nasm
NASMFLAGS = -f bin

all: os-image.bin

os-image.bin: boot.asm
	$(NASM) $(NASMFLAGS) -o $@ $<

clean:
	rm -f os-image.bin
