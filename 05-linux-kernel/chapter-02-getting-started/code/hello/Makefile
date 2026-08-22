# Makefile for hello.ko — Linux Kernel Build (Kbuild)
#
# 用法：
#   make                # 编译 hello.ko
#   make clean          # 清理
#   sudo insmod hello.ko && dmesg | tail
#   sudo rmmod hello    && dmesg | tail
#
# 前置：装好内核构建树
#   Ubuntu/Debian: sudo apt install linux-headers-$(uname -r) build-essential
#   Fedora/RHEL:   sudo dnf install kernel-devel kernel-headers
#   Arch:           sudo pacman -S linux-headers
#
# -C 指向内核源码树（不是 libc！），Kbuild 会回头用 M=$(PWD) 编译本目录
# obj-m 告诉 Kbuild：把 hello.o 编成可加载模块 hello.ko
# （obj-y 则编进内核镜像 built-in，需要内核整体重编）

obj-m += hello.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
