# Ch 5 内核同步 · Kernel Synchronization

> **Understanding the Linux Kernel** 3rd · Bovet & Cesati · **🔴 HFT 精读**  
> 并发执行与临界区保护 — 单核与 SMP 都绕不开

---

## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **大内核锁 (BKL)** | **已删除**（2.6.37 完全移除） | [The BKL lives on](https://lwn.net/Articles/400542/) |
| RCU 基础版 | Tree RCU、Sleepable RCU、Tasks RCU 大幅演进 | [What is RCU?](https://lwn.net/Articles/262464/) (Paul McKenney) |
| `read_lock()` | 仍存在，但 RCU 更推荐用于读多写少 | [Tree RCU](https://lwn.net/Articles/305782/) |
| `atomic_t` | 仍存在，新增 `refcount_t`（防溢出） | [refcount_t](https://lwn.net/Articles/715037/) |
| 顺序锁 | 概念不变，但实现细节有更新 | [Kernel doc: locking](https://docs.kernel.org/locking/) |

> **原则**：同步原语的概念（自旋锁/信号量/RCU/顺序锁）不变，但 BKL 已删、RCU 大幅演进，务必补 LWN 文章。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 内核抢占 | [notes/section-2-内核抢占.md](./notes/section-2-内核抢占.md) |
| 3. 基础原语 | [notes/section-3-基础同步原语.md](./notes/section-3-基础同步原语.md) |
| 4. 自旋锁 | [notes/section-4-自旋锁.md](./notes/section-4-自旋锁.md) |
| 5. 顺序锁与 RCU | [notes/section-5-顺序锁与RCU.md](./notes/section-5-顺序锁与RCU.md) |
| 6. 信号量与完成变量 | [notes/section-6-信号量与完成变量.md](./notes/section-6-信号量与完成变量.md) |
| 7. 选型原则与内核实例 | [notes/section-7-选型与实例.md](./notes/section-7-选型与实例.md) |

---

## 相关

- 上一章：[chapter-04-interrupts-and-exceptions/](../chapter-04-interrupts-and-exceptions/)
- 下一章：[chapter-06-timing/](../chapter-06-timing/)
- 衔接：[chapter-07-process-scheduling.md](../chapter-07-process-scheduling.md) · [chapter-03-processes/](../chapter-03-processes/)
- [OUTLINE.md](../OUTLINE.md) · [LEARNING_PLAN.md](../LEARNING_PLAN.md)
