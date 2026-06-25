# Day 2 · 第一天极简 Makefile（配方行首必须是 Tab）
# 说明：notes/section-2.4-Makefile-入门.md
# 源码：helloos.nas（.nas 后缀不用改，只换 nask → nasm 命令）

NASM = nasm
NASMFLAGS = -f bin

all: os-image.bin

os-image.bin: helloos.nas
	$(NASM) $(NASMFLAGS) -o $@ $<

clean:
	rm -f os-image.bin
