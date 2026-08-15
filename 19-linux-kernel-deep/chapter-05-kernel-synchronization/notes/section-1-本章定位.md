## 1. 本章定位

> **ULK Ch 5 Kernel Synchronization** · 保护共享数据，避免竞态

---

### 一、本章讲什么

Ch 4 讲了中断/异常导致 **内核控制路径交错**。本章回答：

- 交错访问共享数据时如何 **同步**？
- **内核抢占** 对同步有什么影响？
- **自旋锁 vs 信号量 vs RCU** 何时用哪个？

单处理器和多处理器 **都需要** 同步 — SMP 下竞态更常见，但中断嵌套在单核上同样危险。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-内核抢占.md) | 2.6 内核抢占、不可抢占场景 |
| [3](./section-3-基础同步原语.md) | per-CPU、原子、屏障、关中断 |
| [4](./section-4-自旋锁.md) | spinlock、读写自旋锁 |
| [5](./section-5-顺序锁与RCU.md) | seqlock、RCU |
| [6](./section-6-信号量与完成变量.md) | semaphore、completion |
| [7](./section-7-选型与实例.md) | 按访问者选型、BKL/refcount 等实例 |

---

### 三、在 Linux 链上的位置

```
Ch 4  中断与异常  — 控制路径为何交错
Ch 5  内核同步    — 交错时如何保护数据（本章）
Ch 7  进程调度    — 抢占与 schedule() 触发点
Ch 3  等待队列    — 信号量睡眠/唤醒
```

交叉：[05 LKD Ch 9–10](../../../05-linux-kernel/) · HFT 热路径：**spinlock 持锁时间** 直接影响延迟

### 常见陷阱

1. 把 ULK 讲的 BKL（大内核锁）当现代机制——BKL 在 2.6.37 完全移除，现代内核不存在
2. 以为内核同步只需要锁——还需要 memory barrier、原子操作、RCU 等无锁机制
3. 混淆 SMP 和 UP 的同步需求——UP 上自旋锁退化为禁用抢占，但仍需要禁用抢占保护临界区

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** ULK 讲的哪些同步机制在现代内核中已被删除？

<details><summary>答案</summary>

① BKL（Big Kernel Lock，`lock_kernel()`）在 2.6.37 完全移除。② `seqlock` 仍存在但使用场景缩小。③ tasklet 正在被废弃。仍有效的：spinlock、mutex、semaphore、RCU（但版本更新了——Tree RCU、Sleepable RCU）。新增的：`refcount_t`（防溢出）、`percpu_rwsem`、`lockdep`（运行时锁依赖检测）。

</details>

**Q2.** 为什么 UP（单处理器）上仍需要同步机制？

<details><summary>答案</summary>

UP 上没有真正的并行，但有**抢占**——内核代码可能被中断/抢占打断。spinlock 在 UP 上退化为 `preempt_disable()`（防止当前 CPU 被抢占）。但不需要关中断（除非中断也访问该数据）。mutex 在 UP 上只禁用抢占，不做原子操作。

</details>

**Q3.** HFT 用户态为什么也要关心内核同步？

<details><summary>答案</summary>

用户态代码通过 syscall 进入内核，内核中的锁竞争会直接增加 syscall 延迟。例：多线程频繁 `futex` → 内核 `futex` lock 竞争 → 延迟抖动。解决：① 减少系统调用频率（batching）。② 用无锁数据结构（`std::atomic`）替代 `futex`。③ `isolcpus` 减少内核线程竞争。④ `perf lock` 分析锁竞争。

</details>

</details>

---

← [Ch 5 导读](../README.md) · 下一节 [2. 内核抢占](./section-2-内核抢占.md)
> ↔ [LKD Ch09 §9.1 临界区与竞态条件](../../../05-linux-kernel/chapter-09-kernel-sync-intro/notes/section-9.1-临界区与竞态条件.md)
