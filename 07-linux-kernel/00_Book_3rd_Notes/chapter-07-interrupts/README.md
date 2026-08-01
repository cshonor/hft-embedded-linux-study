# Ch 7 中断和中断处理程序 · Interrupts and Interrupt Handlers

> **Linux Kernel Development 3rd** · Robert Love · **精读**

> 本章定位：硬件 **异步打断** → **ISR** → **中断上下文** 规则；**上半部** 与 **下半部** 分工；`request_irq` 与 **关中断** API。HFT **收包延迟、IRQ 绑核、与策略争 cache** 的底层一页。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 中断概念** | IRQ vs 异常 | 异步硬件 · 同步陷阱 |
| **② ISR** | 中断处理程序 | **快** · 中断上下文 |
| **③ 上下半部** | Top / Bottom half | 时限内 vs 可延后 |
| **④ 注册编写** | `request_irq` | `IRQ_HANDLED` · 非可重入 |
| **⑤ 中断上下文** | 与进程上下文对比 | **禁止睡眠** · 中断栈 |
| **⑥ 实现路径** | `do_IRQ` 链 | 硬件 → ISR |
| **⑦ 中断控制** | `local_irq_*` | save/restore |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 中断的概念 | [notes/section-7.1-中断的概念.md](./notes/section-7.1-中断的概念.md) |
| 中断处理程序 | [notes/section-7.2-中断处理程序.md](./notes/section-7.2-中断处理程序.md) |
| 上半部与下半部 | [notes/section-7.3-上半部与下半部.md](./notes/section-7.3-上半部与下半部.md) |
| 注册与编写中断处理程序 | [notes/section-7.4-注册与编写中断处理程序.md](./notes/section-7.4-注册与编写中断处理程序.md) |
| 中断上下文 | [notes/section-7.5-中断上下文.md](./notes/section-7.5-中断上下文.md) |
| 中断处理机制的实现 | [notes/section-7.6-中断处理机制的实现.md](./notes/section-7.6-中断处理机制的实现.md) |
| 中断控制 | [notes/section-7.7-中断控制.md](./notes/section-7.7-中断控制.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| IRQ vs 异常？ | **异步外设** vs **同步 CPU 陷阱** |
| ISR 要求？ | **极快** · **中断上下文** · **不能睡** |
| 为何要下半部？ | 上半部 **时限严** · 重活 **延后**（Ch 8） |
| 怎么注册？ | **`request_irq` / `free_irq`** |
| 共享 IRQ？ | **`IRQF_SHARED`** + `IRQ_NONE`/`HANDLED` |
| 关中断推荐？ | **`local_irq_save` / `restore`** |
| HFT 常做什么？ | **IRQ/NAPI 迁核** · 缩短上半部 · 与用户线程 **NUMA/核隔离** |

---

## 本章学习目标 · 自检

- [ ] 对比 **中断上下文** 与 **syscall 进程上下文**（能否睡眠）
- [ ] 解释 **上半部 / 下半部** 分工
- [ ] 说出 **`IRQ_HANDLED` vs `IRQ_NONE`**
- [ ] 会用 **`local_irq_save/restore`** 的理由
- [ ] 画 **IRQ → do_IRQ → handler → 下半部** 简图
- [ ] 联系 HFT：**策略核与网卡 IRQ 同核** 的后果

---

## 相关章节

- 上一章：[../chapter-06-kernel-data-structures/](../chapter-06-kernel-data-structures/)
- 下一章：[../chapter-08-bottom-halves/](../chapter-08-bottom-halves/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
