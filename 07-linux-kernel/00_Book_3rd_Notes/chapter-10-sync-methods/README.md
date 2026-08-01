# Ch 10 内核同步方法 · Kernel Synchronization Methods

> **Linux Kernel Development 3rd** · Robert Love · **精读**

> 本章定位：Ch 9 理论落地 — **原子、自旋锁、睡眠锁、seqlock、抢占禁用、内存屏障** 等 **具体 API** 与选型。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 原子操作** | `atomic_t` · 位操作 | 同步基础 |
| **② 自旋锁** | spinlock | 短临界区 · 可中断上下文 |
| **③ 读写自旋锁** | rwlock | 读多写少 · 偏袒读者 |
| **④ 信号量** | semaphore | 睡眠 · 长持有 |
| **⑤ 互斥体** | `mutex` | **新代码首选** |
| **⑥ 完成变量** | completion | 事件等待 |
| **⑦ BKL** | 大内核锁 | **禁止新用** |
| **⑧ 顺序锁** | seqlock | 读极多 · 偏袒写者 |
| **⑨ 禁止抢占** | `preempt_disable` | per-CPU 数据 |
| **⑩ 屏障** | `rmb`/`wmb`/`mb` | 防重排序 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 原子操作 | [notes/section-10.1-原子操作.md](./notes/section-10.1-原子操作.md) |
| 自旋锁 | [notes/section-10.2-自旋锁.md](./notes/section-10.2-自旋锁.md) |
| 读-写自旋锁 | [notes/section-10.3-读-写自旋锁.md](./notes/section-10.3-读-写自旋锁.md) |
| 信号量 | [notes/section-10.4-信号量.md](./notes/section-10.4-信号量.md) |
| 互斥体 | [notes/section-10.5-互斥体.md](./notes/section-10.5-互斥体.md) |
| 完成变量 | [notes/section-10.6-完成变量.md](./notes/section-10.6-完成变量.md) |
| 大内核锁 | [notes/section-10.7-大内核锁.md](./notes/section-10.7-大内核锁.md) |
| 顺序锁 | [notes/section-10.8-顺序锁.md](./notes/section-10.8-顺序锁.md) |
| 禁止抢占 | [notes/section-10.9-禁止抢占.md](./notes/section-10.9-禁止抢占.md) |
| 排序和屏障 | [notes/section-10.10-排序和屏障.md](./notes/section-10.10-排序和屏障.md) |
| 选型速查（Ch 9 + Ch 10） | [notes/section-10.11-选型速查Ch-9--Ch-10.md](./notes/section-10.11-选型速查Ch-9--Ch-10.md) |

---

## 本章小结

| 机制 | 睡眠？ | 中断可用？ | 要点 |
|------|--------|------------|------|
| atomic | — | 是 | 基础 |
| spinlock | 否（自旋） | 是（+irqsave） | **不可递归** |
| rwlock | 否 | 是 | **偏袒读者** |
| semaphore | 是 | **否** | 可计数 |
| mutex | 是 | **否** | **新代码首选** |
| completion | 等事件 | 视 API | vfork 等 |
| seqlock | — | 读者无锁快路径 | **偏袒写者** |
| preempt_disable | — | 谨慎 | **per-CPU** |
| barriers | — | 是 | 防重排 |

---

## 本章学习目标 · 自检

- [ ] 区分 **spinlock vs mutex** 的上下文与持有时间
- [ ] 解释 ISR 中为何 **`spin_lock_irqsave`**
- [ ] 对比 **rwlock（偏读）** 与 **seqlock（偏写）**
- [ ] 说出 **mutex 四条严格规则**
- [ ] 知道 **BKL 禁止新用**
- [ ] 各举 **`wmb`/`rmb`/`mb`** 适用场景
- [ ] HFT：热路径 **原子 > 短自旋 > mutex 睡眠**

---

## 相关章节

- 上一章：[../chapter-09-kernel-sync-intro/](../chapter-09-kernel-sync-intro/)
- 下一章：[../chapter-11-timers/](../chapter-11-timers/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
