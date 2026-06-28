# Ch 11 · 定时器和 ACPI

> **原书第 11 章** · HFT **🔴** · 官方源码标签 `osbook_day11`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **时间感知成熟：** APIC **定时中断** · **TimerManager** · **ACPI 校准** · **RSDP**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 重构** | `InitializeXXX()` · **`std::deque`** | 内核工程化 |
| **② 中断** | APIC **periodic** · 向量 **0x41** | 告别轮询读定时器 |
| **③ 多定时器** | **`TimerManager`** · **`priority_queue`** | `kTimerTimeout` Message |
| **④ 校准** | **ACPI PM** 3.579545 MHz · **RSDP** | APIC tick → 真实时间 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 源码重构 | [notes/section-2-源码重构.md](./notes/section-2-源码重构.md) |
| 3. APIC 定时器中断与 TimerManager | [notes/section-3-APIC定时器与TimerManager.md](./notes/section-3-APIC定时器与TimerManager.md) |
| 4. 优先级队列与多定时器 | [notes/section-4-优先级队列与多定时器.md](./notes/section-4-优先级队列与多定时器.md) |
| 5. ACPI PM 定时器校准 | [notes/section-5-ACPI-PM定时器校准.md](./notes/section-5-ACPI-PM定时器校准.md) |
| 6. RSDP 解析与小结 | [notes/section-6-RSDP解析与小结.md](./notes/section-6-RSDP解析与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **~1ms tick** · 多逻辑定时器 · **APIC 频率校准** · 解析 **RSDP** |
| 与 01 川合 OS 对照？ | 01 **Day 12 PIT/定时器**；Mikan **APIC + ACPI PM** |
| 与 Linux / CSAPP 对照？ | `jiffies`/hrtimer 雏形 — [05-LKD](../../../05-Linux-Kernel-Development/) · [ULK 定时](../../../06-Understanding-Linux-Kernel/) |

**本章目的：** **时钟中断 + 超时调度** — 为 Ch 13 **抢占式多任务** 奠基。

---

## 本章学习目标 · 自检

- [ ] 配置 APIC **periodic** 模式与 **0x41** 向量 ISR
- [ ] 理解 **`volatile tick_`** 与编译器优化陷阱
- [ ] 用 **`priority_queue`** 管理多定时器 · **`kTimerTimeout`**
- [ ] 用 **ACPI PM** 校准 Local APIC 频率
- [ ] 从 **UEFI SystemTable** 解析 **RSDP** 签名校验

---

## 相关

- 上一章：[../chapter-10-window/](../chapter-10-window/)
- 下一章：[../chapter-12-keyboard/](../chapter-12-keyboard/)
- 前置：[../chapter-07-interrupt-fifo/](../chapter-07-interrupt-fifo/) · [../chapter-09-layers/](../chapter-09-layers/)
- 后续：[../chapter-13-multitask1/](../chapter-13-multitask1/) 🔴
