# 第一个内核模块：Hello World LKM

> 本章书外补充（嵌入式课程送的第一个例子，正好补 LKD Ch2 概念落地用）。
> 代码：[`code/hello/hello.c`](../code/hello/hello.c) · [`code/hello/Makefile`](../code/hello/Makefile)
> 前置：[§2.4 内核开发的特点](./section-2.4-内核开发的特点.md)（`gnu11` / GNU C / 无 libc）

---

## 0. 先破除「没法跑」——它和你写的 hello.c 不是一回事

你做嵌入式时人家给的第一个例子是这个 `hello.c`，又说「没法跑」——不是不能跑，是**跑的方式完全不同**。把它和你用户态的 `hello.c` 拆开对照，立刻清楚：

| | 用户态 hello.c | 内核模块 hello.c |
|--|----------------|-------------------|
| 编译产物 | ELF **可执行**（`ET_EXEC`），有 `main` | ELF **可重定位**（`ET_REL`，`.ko`），**没有 main** |
| 怎么"跑" | shell `execve("./hello")` → 内核 `sys_execve` 起新进程 | `insmod hello.ko` → 内核把 `.ko` 链接进**自己**地址空间 |
| 谁调你 | C 运行时 `__libc_start_main` → `main` | 内核 `init_module` 系统调用 → 你登记的 `module_init` 函数 |
| 跑在哪 | 新进程，有自己的 `current`、用户栈 | **没有新进程**——借用 `insmod` 进程的上下文执行 |
| 看输出 | 直接打到终端 | 写进内核环形缓冲区，要 `dmesg` 读 |
| 退出 | `return 0` 进程结束 | **init 返回 = 加载完成**，模块继续驻留；`rmmod` 才调 `exit` |

一句话总结：**用户态程序跑起来产生一个新进程，内核模块是把一段代码"焊"进正在运行的内核里**。init 函数返回后那段代码还活着，等着被调用或被 rmmod 卸掉。

这就是「没法 `./hello.ko` 跑」的根因——它是给内核吃的零件，不是给 shell 跑的程序。

---

## 1. 逐行拆解

### 头文件三件套

```c
#include <linux/init.h>    /* module_init / __init / __exit        */
#include <linux/module.h>  /* MODULE_LICENSE / MODULE_AUTHOR 等    */
#include <linux/kernel.h>  /* printk, KERN_INFO                   */
```

**注意**：`<linux/xxx.h>` 这些头不在你用户态的 `/usr/include` 里——它们是**内核源码树**里的头（`include/linux/`、`arch/<arch>/include/`）。Makefile 里 `-C $(KDIR)` 让 Kbuild 用内核自己的头，**绝不**碰 libc。这一点 [§2.4](./section-2.4-内核开发的特点.md) 那张「ISO C / GNU C / glibc / 内核」三层表里讲过。

### 模块元信息宏

```c
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple hello world kernel module");
MODULE_VERSION("0.01");
```

`modinfo hello.ko` 能看到这些字段。**`MODULE_LICENSE` 必须写**——不写或写非 GPL 兼容字符串（如 `"Proprietary"`），内核会把自己标成 **tainted**，`EXPORT_SYMBOL_GPL` 的符号就拿不到，kprobes、lockdep 等调试设施也会部分拒绝配合。这就是为什么几乎所有 LKD 例子里都挂着 `MODULE_LICENSE("GPL")`。

### init / exit 函数 + 两个 section 标记

```c
static int __init hello_init(void)  { ... return 0; }
static void __exit hello_exit(void) { ... }

module_init(hello_init);
module_exit(hello_exit);
```

- `static`：符号不对外导出（链接属性等同文件级 `static`，参考 [01-c-language/3.6.2 内部链接](../../../01-c-language/02-Pointers-on-C/ch03-data/3.6-linkage/3.6.2-内部链接.md)）。内核模块里的"私有"就靠这个。
- `__init`：展开成 `__section(".init.text")`。加载完执行一次后，这段代码所在页可以释放回伙伴系统——模块 init 跑完就丢掉它，省内存。
- `__exit`：展开成 `__section(".exit.text")`。**编进内核 built-in 时链接器整段丢掉**（built-in 永不卸载，exit 永不执行）；可卸载模块才保留它，`rmmod` 时调用。
- `module_init` / `module_exit`：宏，把函数指针登记进模块的 `.modinfo` 段，insmod/rmmod 从这里找入口。
- `init` 返回 0 = 加载成功；返回非 0 = 失败，内核自动回滚已注册的资源（已 `request_irq` 的会 `free_irq` 等）。

### printk + KERN_INFO

```c
printk(KERN_INFO "Hello, World! I am a kernel module.\n");
```

`printk` ≠ `printf`：内核**没有 stdio**，`printf` 不存在。`printk` 把消息写进内核环形缓冲区（`log_buf`，默认 64KB～1MB），用户态 `dmesg` 或 `/dev/kmsg` 读出来。

`KERN_INFO` 是字符串字面量 `"<6>"`（日志等级 6）。**只有**消息等级（数值）**低于**当前 `console_loglevel`（数值高于阈值）才会同时打到控制台——所以有时 `dmesg` 看得到、控制台没动静，是等级问题。

---

## 2. 怎么编译

Makefile（见同目录）核心两行：

```makefile
obj-m += hello.o        # 把 hello.o 编成可加载模块 hello.ko
KDIR ?= /lib/modules/$(shell uname -r)/build
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
```

- `obj-m`：Kbuild 约定——`obj-m` = 编成模块（`.ko`），`obj-y` = 编进内核镜像（built-in，需要重编整个内核）。
- `-C $(KDIR)`：切到内核源码树目录跑 make，而不是用 libc/gcc 默认路径。
- `M=$(PWD)`：让 Kbuild 回头来编译本目录的源文件。

**前置**：必须装内核构建树，否则 `make` 找不到 `/lib/modules/$(uname -r)/build`：

```bash
# Ubuntu/Debian
sudo apt install linux-headers-$(uname -r) build-essential
# Fedora/RHEL
sudo dnf install kernel-devel kernel-headers
# Arch
sudo pacman -S linux-headers
```

`make` 跑完会生成 `hello.ko`、`hello.mod.c`、`hello.mod.o` 等一堆中间产物——其中 `hello.ko` 就是要 insmod 的那个。

---

## 3. 怎么加载 / 卸载 / 看输出

```bash
sudo insmod ./hello.ko     # 加载
dmesg | tail               # 应该看到 "Hello, World! I am a kernel module."
sudo rmmod hello           # 卸载（用模块名，不带 .ko）
dmesg | tail               # 应该看到 "Goodbye, World! Module unloaded."
```

- `lsmod`：看当前加载的模块列表（读 `/proc/modules`）。
- `modinfo hello.ko`：看模块元信息（就是 `MODULE_*` 宏登记的字段）。
- `rmmod` 报 `Module hello is in use`：有别的代码在用你模块里的符号（或进程持有引用），先排查谁在用。

---

## 4. 新手常踩的坑（你自己写时遇到对照）

| 现象 | 根因 |
|------|------|
| `make` 报 `Makefile:XXX: *** No rule` | 没装 `linux-headers-$(uname -r)` / `kernel-devel` |
| `insmod` 报 `Invalid module format` | 模块用 A 内核头编的，往 B 内核里插（`vermagic` 不匹配）；开 `CONFIG_MODVERSIONS` 时符号 CRC 也对不上 |
| 加载后 `dmesg` 没东西 | 输出等级低于 console_loglevel 会被控制台吞，但 ring buffer 里**一定**有，`dmesg` 一定能看到 |
| 改了 .c 重编没生效 | 没先 `make clean` 直接 `make`，中间产物没清；或 `insmod` 旧 `.ko` |
| `rmmod` 报 in use | 还有进程在用模块符号，或代码里有未释放的资源 |
| 系统直接卡死 / 重启 | init 里写了野指针、null deref——**内核代码出错不会像用户态那样 segfault 退出进程，而是整个内核 panic / oops**，可能连日志都来不及写 |

最后这条最重要：**用户态程序崩溃死一个进程，内核态代码崩溃死整个机器**。新手写 LKM 必须有这个意识，调试优先用虚拟机（QEMU）而不是物理机。

---

## 5. 和本书 / 整条路线的关系

- **LKD Ch2** 讲"怎么进内核开发"（源码、构建、特点），但第一个能跑的 hello 模块在 Robert Love 的书里出现在 Ch2 末或 Ch3 起——这本笔记跟着概念走，把"能跑的例子"放在 Ch2 末尾作为落地入口，**和 §2.4「内核 C 是什么」紧密绑定**：你看这个 hello.c 用的就是 `gnu11` 风格（`__init`/`__exit` 是 `__attribute__` 的薄封装，[§2.4 GNU C 扩展](./section-2.4-内核开发的特点.md) 那一节）。
- **下游**：LKD Ch3「进程管理」起，模块会开始和内核子系统打交道（注册 `/proc`、`file_operations`、netlink 等），到那时这个 hello 模块就是你做"加一个小 syscall / char device"练习的脚手架。

---

## 6. 自测题

1. `hello.ko` 是 `ET_EXEC` 还是 `ET_REL`？为什么不能 `./hello.ko` 直接跑？
2. `__init` 在**可卸载模块**和**built-in**里行为一样吗？分别讲一下最终 init 代码的命运。
3. `MODULE_LICENSE("GPL")` 不写会怎样？具体到哪个机制？
4. `printk` 输出去哪了？为什么有时控制台看不到、`dmesg` 却看得到？
5. 内核模块里写 `printf` 会发生什么？（提示：链接阶段）
