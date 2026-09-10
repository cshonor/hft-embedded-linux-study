# 01 · 内核模块：从 insmod 到 dmesg 有输出

> **本节讲什么：** 写出第一个 `.ko`，装载进运行中的内核，理解"内核模块"到底是个什么东西。
> **对应动手：** [P5 · C 阶段热身](../../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（内核模块 HelloWorld、`insmod`/`rmmod`）

---

## 要点

| 概念 | 说明 |
|------|------|
| **模块是什么** | 一段可动态插入内核地址空间的**目标代码**（`.ko`），运行在内核态、共用内核地址空间 |
| **`module_init` / `module_exit`** | 装载/卸载入口。不是 `main` |
| **`printk` 不是 `printf`** | 输出进内核环形缓冲区，用 `dmesg` 看；有日志级别（`KERN_INFO` 等） |
| **版本魔法** | 模块必须与**正在运行的内核**同版本同配置编译，否则 `insmod` 直接拒绝 |
| **没有 libc** | 内核里没有 `printf`/`malloc`/文件描述符，只有 `printk`/`kmalloc`/`struct file *` |

---

## 最小可跑模块

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init hello_init(void)
{
    pr_info("hello: loaded\n");
    return 0;
}

static void __exit hello_exit(void)
{
    pr_info("hello: unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wzp");
MODULE_DESCRIPTION("minimal module");
```

```makefile
obj-m += hello.o
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
```

```bash
make && sudo insmod hello.ko && dmesg -T | tail -3
lsmod | grep hello
sudo rmmod hello && dmesg -T | tail -3
```

**注意：** `MODULE_LICENSE("GPL")` 不是可选项——缺了会触发内核 taint，部分 GPL-only 符号（如很多核心 API）直接不对你开放。

---

## 本节笔记（基础概念）

| # | 笔记 |
|---|------|
| 1.1 | [为什么驱动要放在内核里](./1.1-why-drivers-in-kernel.md) |
| 1.2 | [为什么大多数驱动是共享的](./1.2-why-most-drivers-are-shared.md) |
| 1.3 | [驱动生命周期与中断](./1.3-driver-lifecycle-and-irq.md) |
| 1.4 | [为什么不能用 libc · MODULE_LICENSE 后果](./1.4-why-no-libc-and-module-metadata.md) ★ 对应验收 2、3 |

---

## HFT / 嵌入式关联

- **内核态 vs 用户态的分界**：这是 HFT 里"旁路（DPDK/VFIO）还是走内核"的原点问题。走内核 = 用现成协议栈、有上下文切换；旁路 = 自己管 NIC、零拷贝但失去内核设施。见 [13-dpdk](../../13-dpdk)。
- `__init` 标记为**初始化后释放内存**——这种"用完即弃"的思路在延迟敏感代码里同样适用（初始化路径不占用常驻缓存）。

---

## 验收

- [ ] 自写 `.ko` 能 `insmod`/`rmmod`，`dmesg` 有输出
- [ ] 能解释为什么内核模块不能用 libc
- [ ] 知道 `MODULE_LICENSE` 缺失的后果
- [ ] 遇到 `version magic` 报错知道怎么修（重编匹配内核）

---

## 衔接

- **上一步：** [08 · 07-storage-ota](../../08-embedded-boot-build/07-storage-ota)
- **下一步：** [02-char-device](../02-char-device/)
- **卡住查书：** Madieu Ch1–3 · LDD3 Ch1–2、Ch11
