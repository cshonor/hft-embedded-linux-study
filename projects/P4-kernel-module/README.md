# P4 — 可加载内核模块

> 写一个内核模块：字符设备 + kmalloc 追踪 + /proc 统计，把"内核不是黑盒"变成"我能往里加东西"。

## 项目目标

从用户态跨进内核态。亲手注册一个字符设备、用 kmalloc 分配内核内存、通过 /proc 暴露统计，并在出 bug 时用内核调试工具定位。这是进 `07`/`08.5`/`08.6`/`09` 的综合实操。

## 交付物

- [ ] 可加载内核模块（`.ko`），`insmod`/`rmmod` 干净装卸
- [ ] 字符设备：`open`/`read`/`write`/`ioctl`/`release` 全套 file_operations
- [ ] kmalloc 分配缓冲区，追踪每次分配的大小/调用点
- [ ] `/proc/<mod>_stats` 暴露：分配次数、总字节、峰值、读写计数
- [ ] ioctl 接口清零统计 / 设置缓冲区大小
- [ ] 用户态测试程序读写设备 + 读 /proc
- [ ] 用 `dmesg`、`ftrace`、必要时 `kgdb`/`KASAN` 调试

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`07` linux-kernel](../../07-linux-kernel/) | LKD：模块机制、字符设备、file_operations |
| [`08.5` modern-kernel](../../08.5-modern-kernel/) | 现代 5.x/6.x 内核 API（不要照搬 2.6） |
| [`08.6` kernel-debugging](../../08.6-kernel-debugging/) | printk、Kprobes、KASAN、Ftrace、Oops 分析 |
| [`09` linux-mm](../../09-linux-mm/) | Gorman：kmalloc/slab/slub、物理内存分配 |

## 前置

[P3](../P3-http-server/)（用户态系统编程 + C++ 过关）+ [P2.5](../P2.5-c-toolkit/)（GNU C 扩展 + container_of / 链表过关）。

## 学习目标

- 内核空间 vs 用户空间的边界（copy_to_user/copy_from_user）
- 字符设备注册流程（cdev_init/add，设备号）
- 内核内存分配：kmalloc/kfree、GFP 标志、slab 缓存
- /proc 文件系统的 `seq_file` 接口
- 内核模块崩溃的现场保护与调试方法

## 里程碑

1. **M1** hello world 模块干净 insmod/rmmod
2. **M2** 字符设备 open/read/write 跑通
3. **M3** kmalloc 缓冲 + 分配追踪
4. **M4** /proc 统计 + ioctl 控制
5. **M5** 故意写一个 bug（如越界写），用 KASAN/Oops 定位修复

## 参考模块

- [07-linux-kernel/](../../07-linux-kernel/) — LKD Ch2（模块）、字符设备
- [08.5-modern-kernel/](../../08.5-modern-kernel/) — LWN/Bootlin 现代 API（6.x 内核模块接口变化）
- [08.6-kernel-debugging/](../../08.6-kernel-debugging/) — Ch3 printk、Ch5 KASAN、Ch7 Oops、Ch9 Ftrace
- [09-linux-mm/](../../09-linux-mm/) — Ch8 Slab/Slub（kmalloc 的底层）

## 环境

- 树莓派 5（AArch64, Linux 6.1）或 WSL2 自编译内核
- 编译需内核头：`apt install linux-headers-$(uname -r)`
- `make` → `sudo insmod mymod.ko` → `dmesg | tail`
