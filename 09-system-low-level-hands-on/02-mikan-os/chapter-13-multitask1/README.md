# Ch 13 · 多任务处理（1）

> **原书第 13 章** · HFT **🔴** · 官方源码标签 `osbook_day13`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **并发里程碑：** **Context** · **SwitchContext** · **抢占 20ms** · **TaskManager**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 概念** | 事件循环 vs **真多任务** · **Context** | 寄存器+栈 = 运行环境 |
| **② 切换** | **`SwitchContext`** · `call`/`iret` | **合作式** 多任务 |
| **③ 抢占** | 定时器 **20ms** 强制切换 | **时间片** 调度 |
| **④ 管理** | **`Task` / `TaskManager`** · **`vector`** | 任意数量任务 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 多任务与上下文 | [notes/section-2-多任务与上下文.md](./notes/section-2-多任务与上下文.md) |
| 3. SwitchContext 与合作式切换 | [notes/section-3-SwitchContext与合作式切换.md](./notes/section-3-SwitchContext与合作式切换.md) |
| 4. 抢占式多任务与时间片 | [notes/section-4-抢占式多任务与时间片.md](./notes/section-4-抢占式多任务与时间片.md) |
| 5. Task 与 TaskManager | [notes/section-5-Task与TaskManager.md](./notes/section-5-Task与TaskManager.md) |
| 6. 均分时间片问题与小结 | [notes/section-6-均分时间片问题与小结.md](./notes/section-6-均分时间片问题与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **合作式 → 抢占式** · **TaskContext** · **20ms 切片** |
| 与 01 川合 OS 对照？ | 01 **Day 15–16 多任务**；Mikan **长模式 + APIC 抢占** |
| 与 Linux / CSAPP 对照？ | `context_switch` / `schedule` 雏形 — [05-LKD](../../../05-Linux-Kernel-Development/) · [ULK Ch7 调度](../../../06-Understanding-Linux-Kernel/chapter-07-process-scheduling.md) |

**遗留问题：** 任务 **死板均分 20ms** → 鼠标 **卡顿** → **Ch14 优先级/休眠**

---

## 本章学习目标 · 自检

- [ ] 区分 **事件循环多任务** vs **多 Task 并发**
- [ ] 说清 **TaskContext** 与 **SwitchContext**（RIP/RSP）
- [ ] 解释 **合作式** 缺陷与 **抢占式 20ms** 定时器
- [ ] 描述 **Task / TaskManager / vector** 职责

---

## 相关

- 上一章：[../chapter-12-keyboard/](../chapter-12-keyboard/)
- 下一章：[../chapter-14-multitask2/](../chapter-14-multitask2/) 🔴
- 前置：[../chapter-07-interrupt-fifo/](../chapter-07-interrupt-fifo/) · [../chapter-11-timer-acpi/](../chapter-11-timer-acpi/)
