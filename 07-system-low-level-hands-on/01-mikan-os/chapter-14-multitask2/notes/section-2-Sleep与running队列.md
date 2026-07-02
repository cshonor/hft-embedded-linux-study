## 2. Sleep、Wakeup 与 running 队列

---

### 一、从「全体 vector」到 running_

**Ch13：** `TaskManager` 对 **所有 Task** Round-Robin — 含 **已阻塞/无事** 者。

**Ch14：** 维护 **`running_`** — **当前 eligible 运行队列**：

| 操作 | 效果 |
|------|------|
| **Sleep(task)** | 从 **`running_` 移除** — 不再参与轮转 |
| **Wakeup(task)** | **加回 `running_`** — 下次调度可选 |

**调度器：** 只在 **`running_`** 内 **Switch** — 无空转片。

---

### 二、Sleep() 何时调用

| 场景 | 行为 |
|------|------|
| Task **无事可做** | 主循环发现 **无消息** → **Sleep(self)** |
| Task **主动等待** | 等 I/O / 定时器（扩展） |

**效果：** CPU 时间 **让给** 其他 **仍在 running_ 的 Task**。

---

### 三、Wakeup() 何时调用

| 调用者 | 场景 |
|--------|------|
| **ISR / SendMessage 路径** | 硬件事件 → 目标 Task 有活 |
| **TimerManager** | 休眠超时（后续扩展） |
| **其他 Task** | IPC（Ch29） |

**Wakeup 后：** Task 回到 **`running_`** — 下次 **定时 tick 或立即 reschedule** 可运行。

→ 与 Linux **`TASK_RUNNING` / `TASK_INTERRUPTIBLE`** 状态机同构（缩小版）

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 消息队列](./section-3-每任务消息队列与事件驱动.md)
