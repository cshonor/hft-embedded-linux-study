# Bootlin: Linux 内核架构概述

> **来源:** [Bootlin Kernel Training](https://bootlin.com/docs/kernel/)
> **主题:** 内核架构概览
> **对标旧书:** ULK3 Ch1 / LKD3 Ch1-2

---

## 讲义要点

### 内核空间 vs 用户空间

| 特性 | 内核空间 | 用户空间 |
|------|---------|---------|
| 特权级 | Ring 0 (x86) / EL1 (ARM64) | Ring 3 / EL0 |
| 内存访问 | 可访问全部地址空间 | 只能访问自己的地址空间 |
| 系统调用 | 可直接调用内核函数 | 通过 syscall 指令陷入内核 |
| 错误影响 | 整个系统崩溃 | 仅当前进程崩溃 |
| 浮点运算 | 禁止（保存/恢复开销大） | 允许 |

### 内核子系统概览 (6.x)

```
┌────────────────────────────────────────────┐
│              系统调用接口                    │
├────────┬────────┬────────┬────────┬────────┤
│ 进程管理 │ 内存管理 │ 文件系统 │ 网络栈  │ 设备驱动 │
│ (sched) │  (mm)  │  (VFS) │ (net)  │ (drv)  │
├────────┴────────┴────────┴────────┴────────┤
│              块 I/O 层 (block)               │
├────────────────────────────────────────────┤
│              中断 / 定时器 / 同步             │
├────────────────────────────────────────────┤
│              硬件抽象层 (HAL)                │
└────────────────────────────────────────────┘
```

### 6.x 相比 ULK3/LKD3 时代的变化

| 子系统 | 2.6 时代 | 6.x 现代 |
|--------|---------|---------|
| 调度器 | O(1) → CFS | EEVDF (6.6+) |
| 内存管理 | SLAB | SLUB + folio |
| VMA 查找 | 红黑树 | maple tree (6.1+) |
| 页缓存 | page + radix tree | folio + maple tree |
| 块 I/O | 单队列 | blk-mq 多队列 |
| 中断 | hardirq + tasklet | threaded IRQ (tasklet 废弃中) |
| 同步 | ticket spinlock | qspinlock |
| 异步 I/O | AIO | io_uring (5.1+) |

### 内核源码组织 (6.x)

```
linux-6.x/
├── arch/           # 架构相关 (arm64/, x86/, riscv/)
├── kernel/         # 核心子系统 (sched/, fork/, exit.c)
├── mm/             # 内存管理
├── fs/             # 文件系统 (VFS + 具体文件系统)
├── block/          # 块 I/O 层
├── net/            # 网络栈
├── drivers/        # 设备驱动
├── include/        # 内核头文件
├── Documentation/  # 内核文档 (RST 格式)
└── tools/          # 用户空间工具 (perf, bpftool, etc.)
```

---

## 动手实验

```bash
# 1. 获取树莓派 5 内核源码
git clone --depth 1 https://github.com/raspberrypi/linux.git
cd linux
git checkout rpi-6.1.y

# 2. 查看内核版本和编译配置
make ARCH=arm64 kernelversion   # 6.1.63
make ARCH=arm64 defconfig      # 生成 .config
make ARCH=arm64 menuconfig     # 自定义配置

# 3. 编译内核 (树莓派 5)
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image dtbs modules

# 4. 查看当前运行内核信息
uname -r                       # 6.1.63-v8+
cat /proc/version
zcat /proc/config.gz | grep CONFIG_PREEMPT  # 查看编译选项
```

---

## 与旧书差异

| ULK3 讲的 | Bootlin 讲义 |
|-----------|-------------|
| 基于 2.6 源码树 | 跟随最新 LTS (6.1/6.6) |
| 手动浏览源码 | 用 Elixir 交叉索引工具 |
| 无设备树概念 | 设备树是嵌入式核心 |
| 无 BPF | eBPF 是现代观测核心 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 用户空间程序触发系统调用时，CPU 状态发生什么变化？

> x86-64: 执行 `syscall` 指令 → CPU 从 Ring 3 切到 Ring 0，切换到内核栈，执行 entry_SYSCALL_64 → 调用 sys_xxx()。ARM64: 执行 `svc #0` 指令 → CPU 从 EL0 切到 EL1，跳到向量表入口。

**Q2:** 内核中为什么禁止使用浮点运算？

> 内核切换到内核态时不保存浮点寄存器（FPU 上下文），如果内核使用浮点运算会破坏用户空间的 FPU 状态。内核中需要浮点时必须显式 `kernel_fpu_begin()` / `kernel_fpu_end()`，开销很大。

**Q3:** ULK3 的源码浏览方法在现代内核上有什么问题？

> ULK3 基于 2.6 源码，大量函数、结构体已被重命名或删除（如 O(1) 调度器、SLAB 分配器、ticket spinlock）。直接对照 ULK3 在 6.x 源码中查找会找不到。应使用 Elixir (elixir.bootlin.com) 在线交叉索引。

</details>
