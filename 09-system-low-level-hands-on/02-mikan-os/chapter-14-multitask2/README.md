# Ch 14 · 多任务处理（2）

> **原书第 14 章** · HFT **🔴** · 官方源码标签 `osbook_day14`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **调度优化：** **Sleep/Wakeup** · **per-Task 消息** · **Level 0–3** · **Idle Task**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 休眠** | **`running_` 队列** · **Sleep/Wakeup** | 无事不占 CPU |
| **② 事件** | **每 Task `msgs_`** · **SendMessage** | ISR 唤醒主任务 |
| **③ 优先级** | **Level 0–3** · 分级 **running** | 鼠标 **即时抢占** |
| **④ Idle** | **Level 0 · `hlt`** | 队列永空 · 省电 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. Sleep、Wakeup 与 running 队列 | [notes/section-2-Sleep与running队列.md](./notes/section-2-Sleep与running队列.md) |
| 3. 每任务消息队列与事件驱动 | [notes/section-3-每任务消息队列与事件驱动.md](./notes/section-3-每任务消息队列与事件驱动.md) |
| 4. 性能验证 | [notes/section-4-性能验证.md](./notes/section-4-性能验证.md) |
| 5. 任务优先级 Level | [notes/section-5-任务优先级Level.md](./notes/section-5-任务优先级Level.md) |
| 6. Idle Task 与小结 | [notes/section-6-Idle-Task与小结.md](./notes/section-6-Idle-Task与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **优先级 + 事件驱动 Sleep/Wakeup** · 消除 Ch13 鼠标卡 |
| 与 01 川合 OS 对照？ | 01 **Day 16** 休眠/优先级；Mikan **4 级队列 + Idle** |
| 与 Linux / CSAPP 对照？ | runqueue / **SCHED** 雏形 — [ULK 调度](../../../06-Understanding-Linux-Kernel/chapter-07-process-scheduling.md) |

**模型：** **基于优先级的事件驱动多任务调度**

---

## 本章学习目标 · 自检

- [ ] 说清 **Sleep/Wakeup** 与 **`running_`**
- [ ] 解释 **per-Task `msgs_`** 取代 **global main_queue**
- [ ] 描述 **Level 0–3** 调度顺序 · 主任务 **Level 3**
- [ ] 说明 **Idle Task** 为何 **永不 Sleep**

---

## 相关

- 上一章：[../chapter-13-multitask1/](../chapter-13-multitask1/) 🔴
- 下一章：[../chapter-15-terminal/](../chapter-15-terminal/)
- 前置：[../chapter-07-interrupt-fifo/](../chapter-07-interrupt-fifo/)
