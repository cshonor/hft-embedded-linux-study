# Ch 9 内核同步介绍 · An Introduction to Kernel Synchronization

> **Linux Kernel Development 3rd** · Robert Love · **精读**

> 本章定位：**并发为何发生、保护什么、死锁与争用** — 理论铺垫；下一章 **Ch 10** 讲 spinlock、mutex、RCU 等 **具体 API**。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 临界区与竞态** | critical region | **原子性** · **同步** |
| **② 加锁** | locking | **建议性** · 自旋 vs 睡眠 |
| **③ 并发原因** | 五类来源 | IRQ · BH · 抢占 · 睡眠 · SMP |
| **④ 保护什么** | what to lock | **lock data, not code** |
| **⑤ 死锁** | deadlocks | 顺序 · ABBA |
| **⑥ 争用与扩展性** | contention | **锁粒度** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 临界区与竞态条件 | [notes/section-9.1-临界区与竞态条件.md](./notes/section-9.1-临界区与竞态条件.md) |
| 加锁 | [notes/section-9.2-加锁.md](./notes/section-9.2-加锁.md) |
| 并发的原因 | [notes/section-9.3-并发的原因.md](./notes/section-9.3-并发的原因.md) |
| 知道要保护什么 | [notes/section-9.4-知道要保护什么.md](./notes/section-9.4-知道要保护什么.md) |
| 死锁 | [notes/section-9.5-死锁.md](./notes/section-9.5-死锁.md) |
| 争用和可扩展性 | [notes/section-9.6-争用和可扩展性.md](./notes/section-9.6-争用和可扩展性.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 临界区？ | 访问共享数据的代码 — 需 **原子** |
| 竞态？ | 多线程 **可能同时** 进同一临界区 |
| 锁的本质？ | **自愿** 保护 **数据** |
| 五类并发源？ | **中断 · BH · 抢占 · 睡眠 · SMP** |
| 死锁？ | **自死锁 · ABBA** — 固定加锁顺序 |
| 争用 vs 粒度？ | 粗锁害扩展；细锁有无谓开销 — **先简单后优化** |

---

## 本章学习目标 · 自检

- [ ] 定义 **临界区、竞态、同步**
- [ ] 列出 Linux 内核 **五种并发原因** 并各举一例
- [ ] 解释 **「保护数据，不是保护代码」**
- [ ] 画出 **ABBA** 死锁环
- [ ] 区分 **锁争用** 与 **可扩展性**、**锁粒度** 权衡
- [ ] 联系 HFT：热路径 **短临界区**、少争用、多 **per-CPU/无锁**

---

## 相关章节

- 上一章：[../chapter-08-bottom-halves/](../chapter-08-bottom-halves/)
- 下一章：[../chapter-10-sync-methods/](../chapter-10-sync-methods/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
