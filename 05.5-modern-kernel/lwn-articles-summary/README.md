# LWN 文章摘要 — 内核子系统

> 对标 ULK3 / LKD3 过时章节的 LWN.net 深度文章摘要。
> 每篇文章按：原文链接 + 核心观点 + 与旧书差异 + 关键代码变更 + HFT 关联 + 自测题 整理。

## 索引

### 调度器
- [x] [EEVDF 调度器 (6.6+)](01-eevdf-scheduler.md) — lag、eligible time、virtual deadline、latency-nice
- [x] [CFS 原理与历史](02-cfs-history.md) — vruntime、红黑树、nice→weight 映射

### 同步 / RCU
- [x] [RCU 基础](03-rcu-basics.md) — grace period、rcu_read_lock/rcu_assign_pointer/synchronize_rcu
- [x] [RCU 进阶](04-rcu-advanced.md) — Tree RCU、SRCU、expedited RCU、lazy RCU 对比
- [x] [Queued spinlock](05-queued-spinlock.md) — MCS 队列、pending bit、O(N)→O(1) cache bouncing

### 中断
- [x] [IRQ domain 框架](06-irq-domain.md) — hwirq→virq 映射、linear/radix tree/legacy
- [x] [Threaded IRQ](07-threaded-irq.md) — hardirq+thread 分离、PREEMPT_RT、tasklet 弃用

### 块设备 / I/O
- [x] [blk-mq 多队列](08-blk-mq.md) — per-CPU SQ + hardware HQ、I/O scheduler 变化
- [x] [io_uring](09-io-uring.md) — SQ/CQ ring buffer、SQPOLL 零系统调用模式

### 系统调用
- [x] [vDSO](10-vdso.md) — vvar page、零系统调用 gettimeofday/clock_gettime

### 调试与观测
- [x] [现代内核调试工具](11-modern-kernel-debugging.md) — eBPF/ftrace/drgn/crash，对标 LKD3 Ch18 过时内容

> 完整映射表见 [../ref-modern-kernel-resources.md](../ref-modern-kernel-resources.md)
