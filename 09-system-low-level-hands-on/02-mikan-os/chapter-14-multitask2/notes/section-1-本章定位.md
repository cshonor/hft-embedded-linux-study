## 1. 本章定位

> **《从零自制操作系统》Ch 14 多任务处理（2）**

---

### 一、Ch 13 遗留问题

| 问题 | 表现 |
|------|------|
| **Round-Robin 均分 20ms** | Idle Task **空占** 时间片 |
| **主任务（鼠标）** 被延迟 | **卡顿** · 交互劣化 |
| **CPU 浪费** | 无事 Task 仍被调度 |

**本章目标：** **性能优化** — **休眠/唤醒 + 优先级** → **流畅 UI + 高效后台**

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **`running_` 队列** | 仅 **可运行** Task |
| **Sleep / Wakeup** | 移出/加入队列 |
| **per-Task `msgs_`** | 事件驱动 · **SendMessage** |
| **Level 0–3** | 分级运行队列 · **高优先先跑** |
| **Idle Task** | Level 0 · **`hlt`** · 永不 Sleep |

---

### 三、目标调度模型

```
硬件事件 → ISR SendMessage → Wakeup(高优先级主任务)
主任务处理完 → 队列空 → Sleep
调度器 → 选最高非空 Level 的 Task
全 Sleep → Idle hlt（兜底）
```

→ [Ch13 均分片问题](../chapter-13-multitask1/notes/section-6-均分时间片问题与小结.md)

---

← [Ch 14 导读](../README.md) · 下一节 [2. Sleep](./section-2-Sleep与running队列.md)
