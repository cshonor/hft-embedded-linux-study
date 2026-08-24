obj-m += hello.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean modules test

# 默认只编用户态，WSL 没装 headers 也能 make test。
all:
	$(MAKE) -C userspace

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

test:
	$(MAKE) -C userspace test

clean:
	-$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(MAKE) -C userspace clean
