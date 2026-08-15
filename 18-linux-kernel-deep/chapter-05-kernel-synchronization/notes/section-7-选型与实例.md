## 7. 同步选型原则与内核实例

---

### 一、基本原则

**尽可能保持高并发** — 锁粒度越细、读写分离越好，但复杂度越高。

按 **谁访问数据结构** 选型：

| 访问者 | 典型保护 |
|--------|----------|
| **仅异常路径**（如 syscall） | **信号量**（可睡眠） |
| **异常 + 中断** | **local_irq_disable + 自旋锁** |
| **异常 + 中断 + 其他 CPU** | **自旋锁**（+ 关中断若 ISR 也访问） |
| **读多写少** | RW 锁、seqlock、**RCU** |
| **每 CPU 私有** | **per-CPU 变量**，无锁 |

还要区分访问来自：[Ch 4 异常 / 中断 / 可延迟函数](../../chapter-04-interrupts-and-exceptions/)

---

### 二、内核中的实际实例

| 实例 | 机制 | 保护什么 |
|------|------|----------|
| **引用计数器** | **原子操作** | 资源 alloc/free 竞态 |
| **大内核锁 (BKL)** | 粗粒度信号量/自旋锁 | 2.6 早期 **整个内核**（已废弃） |
| | 特殊：`schedule()` 时 **自动释放**，返回时再取 | |
| **内存描述符** | **读/写信号量** | 进程间 `mm_struct` 并发 |
| **Slab 缓存列表** | 信号量 | 分配器链表 |
| **索引节点 (Inode)** | 信号量 | 目录/文件元数据并发 |

Modern 内核：**BKL 已移除**，锁更细粒度；读 ULK 时理解 **设计思想**，不必照搬 BKL。

---

### 三、后续章节索引

| Ch 5 主题 | 继续读 |
|-----------|--------|
| 调度、抢占点 | [Ch 7 进程调度](../../chapter-07-process-scheduling.md) 🔴 |
| 控制路径、ISR | [Ch 4 中断与异常](../../chapter-04-interrupts-and-exceptions/) 🔴 |
| 睡眠、唤醒 | [Ch 3 等待队列](../../chapter-03-processes/notes/section-4-组织与查找.md) |
| Slab 分配 | [Ch 8 内存管理](../../chapter-08-memory-management.md) 🔴 |
| VFS inode | [Ch 12 VFS](../../chapter-12-VFS.md) ⚪ |
| LKD 现代锁 API | [05 LKD Ch 9–10](../../../05-linux-kernel/) |

### 常见陷阱

1. 在所有场景都用 spinlock——spinlock 只适合极短临界区，长临界区用 mutex（可睡眠不浪费 CPU）
2. 以为 RCU 总是最优——RCU 读端无开销但写端开销大（等 grace period），写频繁时不适合
3. 忽略 lockdep——`CONFIG_LOCKDEP=y` 在开发阶段能检测死锁/锁顺序反转，生产阶段关掉

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 给定一个场景，如何选择同步原语？

<details><summary>答案</summary>

中断上下文 + 短临界区 → `spinlock_irqsave()`。softirq + 短临界区 → `spinlock_bh()`。进程上下文 + 短临界区（<1us） → `spinlock()`。进程上下文 + 长临界区（>1us） → `mutex()`。读多写少 + 简单数据 → `seqlock` 或 RCU。读多写少 + 复杂数据结构 → RCU。一次性等待 → `completion`。引用计数 → `refcount_t`。

</details>

**Q2.** `lockdep` 能检测哪些问题？怎么使用？

<details><summary>答案</summary>

检测：① 死锁（A→B 和 B→A 锁顺序反转）。② 同一个锁重复加锁。③ 在中断上下文中持有可睡眠锁。④ 锁的 IRQ 安全性不匹配（进程上下文用 `spinlock()`，中断上下文用 `spinlock_irqsave()` 但两者锁定同一 `spinlock` → 死锁风险）。使用：`CONFIG_LOCKDEP=y` 编译内核，`echo 1 > /proc/sys/kernel/lock_stat` 开启统计，`cat /proc/lock_stat` 查看结果。

</details>

**Q3.** HFT 用户态有没有类似 lockdep 的工具？

<details><summary>答案</summary>

有：① ThreadSanitizer (`-fsanitize=thread`)：编译时插桩，运行时检测数据竞争。② Helgrind (Valgrind)：无需重编译，但慢 20-50x。③ `perf lock`：内核级锁竞争分析（含 `futex`）。④ `bpftrace`：`tracepoint:syscalls:sys_enter_futex` 追踪 futex 等待。HFT 开发应 CI 中跑 TSan，上线前跑 `perf lock` 确认无异常锁竞争。

</details>

</details>

---

← [6. 信号量与完成变量](./section-6-信号量与完成变量.md) · 下一章 [Ch 6 计时](../../chapter-06-timing.md)
> ↔ [LKD Ch10 §10.11 选型速查Ch-9--Ch-10](../../../05-linux-kernel/chapter-10-sync-methods/notes/section-10.11-选型速查Ch-9--Ch-10.md)
