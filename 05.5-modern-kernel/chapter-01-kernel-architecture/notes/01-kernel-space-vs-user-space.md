# 内核空间 vs 用户空间与子系统概览

> 来源: Bootlin Kernel Training
> 对标旧书: ULK3 Ch1 / LKD3 Ch1-2

---

## 特权级与隔离

| 特性 | 内核空间 | 用户空间 |
|------|---------|---------|
| 特权级 | Ring 0 (x86) / EL1 (ARM64) | Ring 3 / EL0 |
| 内存访问 | 可访问全部地址空间 | 只能访问自己的地址空间 |
| 系统调用 | 可直接调用内核函数 | 通过 syscall 指令陷入内核 |
| 错误影响 | 整个系统崩溃 | 仅当前进程崩溃 |
| 浮点运算 | 禁止（保存/恢复开销大） | 允许 |
| 页表 | 内核映射 + 用户映射 | 仅用户映射 |

### 系统调用陷入流程

```c
// x86-64: syscall 指令
// 用户空间 → syscall → Ring 3→Ring 0 → entry_SYSCALL_64 → sys_xxx()

// ARM64: svc #0 指令
// 用户空间 → svc #0 → EL0→EL1 → vector table → el0_sync → sys_xxx()

// 系统调用号通过寄存器传递 (x8 on ARM64, rax on x86-64)
// 参数通过 x0-x5 / rdi,rsi,rdx,r10,r8,r9 传递
```

### 内核栈 vs 用户栈

```c
// 每个线程有两个栈:
// 1. 用户栈 — 在用户空间地址空间
// 2. 内核栈 — THREAD_SIZE (ARM64: 16KB or 32KB with VMAP_STACK)

// 线程信息保存在内核栈底部 (旧) 或 task_struct 中 (新)
struct task_struct {
    struct thread_info thread_info;  // 6.x: 已移入 task_struct
    void *stack;                     // 指向内核栈
    // ...
};

// 6.x 变化: thread_info 不再放栈底，直接嵌入 task_struct
// CONFIG_VMAP_STACK=y: 内核栈用 vmalloc 分配，带守卫页
```

---

## 内核子系统概览 (6.x)

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

| 子系统 | 2.6 时代 | 6.x 现代 | 变化原因 |
|--------|---------|---------|----------|
| 调度器 | O(1) → CFS | EEVDF (6.6+) | CFS vruntime 精度问题 |
| 内存管理 | SLAB | SLUB + folio | SLUB 更省内存，folio 替代 page |
| VMA 查找 | 红黑树 | maple tree (6.1+) | 红黑树并发性能差 |
| 页缓存 | page + radix tree | folio + maple tree | 减少 page 结构体数量 |
| 块 I/O | 单队列 | blk-mq 多队列 | NVMe 多队列需求 |
| 中断 | hardirq + tasklet | threaded IRQ | tasklet 废弃中 |
| 同步 | ticket spinlock | qspinlock | 减少 cache bouncing |
| 异步 I/O | AIO | io_uring (5.1+) | AIO 接口复杂且限制多 |

---

## HFT 关联

| 概念 | HFT 应用 |
|------|----------|
| 用户/内核隔离 | 交易线程在用户空间，通过 syscall 与内核交互（减少 syscall = 减少延迟） |
| 内核栈大小 | 16KB 内核栈足够，但深递归会溢出（驱动开发注意） |
| VMAP_STACK | 守卫页检测栈溢出，HFT 自定义驱动开发期必开 |
| 子系统变化 | EEVDF/qspinlock/blk-mq 都在减少锁争用和调度延迟 |

> **HFT 原则：** 交易热路径尽量不陷入内核（用户态网卡 DPDK/AF_XDP），减少 syscall 次数。

---

## 自测题

<details>
<summary>Q1: 用户空间程序触发系统调用时，CPU 状态发生什么变化？</summary>

x86-64: 执行 `syscall` 指令 → CPU 从 Ring 3 切到 Ring 0，切换到内核栈，执行 entry_SYSCALL_64 → 调用 sys_xxx()。ARM64: 执行 `svc #0` 指令 → CPU 从 EL0 切到 EL1，跳到向量表入口 el0_sync。两种架构都会保存用户态 PC 和 PSTATE，切换到内核栈。
</details>

<details>
<summary>Q2: 内核中为什么禁止使用浮点运算？</summary>

内核切换到内核态时不保存浮点寄存器（FPU 上下文），如果内核使用浮点运算会破坏用户空间的 FPU 状态。内核中需要浮点时必须显式 `kernel_fpu_begin()` / `kernel_fpu_end()`，开销很大（保存/恢复大量寄存器）。HFT 自定义驱动中如果需要浮点计算（如统计计算），应考虑用整数定点替代。
</details>

<details>
<summary>Q3: 6.x 内核用 maple tree 替代红黑树管理 VMA，原因是什么？</summary>

红黑树在多线程并发访问时需要锁保护，成为多核扩展瓶颈。maple tree 是 B-tree 变体，支持 RCU 并发读取，写入时使用细粒度锁。在多核服务器上 VMA 查找性能显著提升。HFT 进程通常 VMA 数量不多，感知不明显，但内核整体性能改善。
</details>

<details>
<summary>Q4: CONFIG_VMAP_STACK 是什么？为什么 HFT 驱动开发需要它？</summary>

VMAP_STACK 让内核栈通过 vmalloc 分配，在栈溢出时触发守卫页的 page fault 而不是静默破坏内存。HFT 自定义内核驱动如果存在栈溢出 bug，VMAP_STACK 能立即报错而非产生难以追踪的内存损坏。开发期必开，生产期可关闭以省一点内存。
</details>

---

## 交叉引用

- [02-kernel-source-organization.md](./02-kernel-source-organization.md) — 内核源码目录树与编译
- [chapter-02-scheduler](../chapter-02-scheduler/) — EEVDF 调度器详解
- [chapter-05-interrupt-management](../chapter-05-interrupt-management/) — 中断管理现代实现
