# 最小内核模块 Makefile
#
# 两种写法：
#   obj-m        = 编译成可加载模块（.ko）—— 本例
#   obj-y        = 编进内核镜像（built-in），需要重编整个内核
#
# -C $(KDIR) M=$(PWD) 的含义：-C 切到内核构建目录去读它的 Makefile 规则，
# 再让内核构建系统回到 M= 指定的目录来编我们的模块。
# 这就是为什么必须有「内核头文件（build 目录）」—— 自己装个 gcc 是不够的。

obj-m := hello.o

KDIR  ?= /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load: all
	sudo insmod hello.ko
	dmesg | tail -3

unload:
	sudo rmmod hello
	dmesg | tail -2

info:
	modinfo hello.ko

.PHONY: all clean load unload info
