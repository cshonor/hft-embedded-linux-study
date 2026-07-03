# Ch 7 · 中断和 FIFO

> **原书第 7 章** · HFT **🔴** · 官方源码标签 `osbook_day07`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **事件模型升级：** 轮询 → **中断 + MSI** · **FIFO** · **事件循环**

---

## 本章定位 · 前后章关系

| | |
|---|---|
| **本章干什么** | 建立 **IDT / 中断处理** 与 **FIFO 队列**；键鼠等事件 **异步投递** 到主循环。 |
| **全书作用** | **内核骨架 · 事件模型**（🔴 HFT 精读）— 现代 OS 不再轮询硬件；Ch13+ 多任务、Ch11 定时器都建在本章事件链上。 |
| **← 前置** | [Ch6 鼠标轮询](../chapter-06-mouse-pci/) — 体会轮询的局限 |
| **→ 后续** | [Ch8 内存管理](../chapter-08-memory/) — OS 自持物理页分配 |

---
### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① ISR** | `__attribute__((interrupt))` · **EOI** | 硬件主动通知 CPU |
| **② IDT** | 向量 0–255 · **`lidt`** | 向量 → 处理程序 |
| **③ MSI** | PCI **Message Signaled Interrupt** | xHC 现代中断路径 |
| **④ FIFO** | **`ArrayQueue<Message>`** | ISR 快 · 主循环慢 |
| **⑤ 并发** | **`cli`/`sti`** · **`hlt`** | 队列安全 · 空闲休眠 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 中断处理程序与 EOI | [notes/section-2-中断处理程序与EOI.md](./notes/section-2-中断处理程序与EOI.md) |
| 3. IDT 与 lidt | [notes/section-3-IDT与lidt.md](./notes/section-3-IDT与lidt.md) |
| 4. MSI 中断配置 | [notes/section-4-MSI中断配置.md](./notes/section-4-MSI中断配置.md) |
| 5. FIFO 与 ArrayQueue | [notes/section-5-FIFO与ArrayQueue.md](./notes/section-5-FIFO与ArrayQueue.md) |
| 6. 事件循环与并发控制 | [notes/section-6-事件循环与并发控制.md](./notes/section-6-事件循环与并发控制.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **IDT + MSI** · ISR **入队** · 主循环 **出队绘图** |
| 与 02 川合 OS 对照？ | 01 **Day 9–11 PIC/IDT**；Mikan 加 **MSI + FIFO 事件环** |
| 与 Linux / CSAPP 对照？ | 顶半部/底半部雏形 — [04-Linux-Kernel-Development 中断](../../../04-Linux-Kernel-Development/) · [ULK](../../../05-Understanding-Linux-Kernel/) |

**本章目的：** **被动唤醒 + 异步队列** — 为 Ch 11 **定时器**、Ch 13 **多任务** 打地基。

---

## 本章学习目标 · 自检

- [ ] 对比 **轮询 vs 中断**；会写 ISR 并 **EOI**
- [ ] 构建 **IDT** 并用 **`lidt`** 加载
- [ ] 说清 **MSI** 与 legacy IRQ 线区别 |
- [ ] 实现 **定长 FIFO**；ISR 只 **Push Message** |
- [ ] 用 **`cli`/`sti`** 保护队列；空队列 **`hlt`** |

---

## 相关

- 上一章：[../chapter-06-mouse-pci/](../chapter-06-mouse-pci/)
- 下一章：[../chapter-08-memory/](../chapter-08-memory/) 🔴
- 后续：[../chapter-11-timer-acpi/](../chapter-11-timer-acpi/) · [../chapter-13-multitask1/](../chapter-13-multitask1/)
- 对照：[01 Day 9 IDT](../../02-30days-os/day-09-idt/)
