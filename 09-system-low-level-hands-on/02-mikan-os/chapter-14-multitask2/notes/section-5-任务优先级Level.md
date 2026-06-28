## 5. 任务优先级 Level

---

### 一、四级 Level（0–3）

| Level | 典型 Task | 说明 |
|-------|-----------|------|
| **3（最高）** | **Main** — 鼠标/键盘/GUI | **关键交互** |
| **2** | （预留 / 驱动） | |
| **1** | **TaskB** · 后台计算 | |
| **0（最低）** | **Idle** | 仅 `hlt` — §6 |

**数据结构：** **`running_[level]`** — 每级 **独立运行队列**（或 deque 列表）。

---

### 二、调度策略

```
Schedule():
    for level from 3 down to 0:
        if running_[level] not empty:
            pick next task in this level
            SwitchContext to it
            return
    // 不应到达 — Idle 总在 level 0
```

| 规则 | 效果 |
|------|------|
| **总是最高非空 Level** | Main **Wakeup 后立刻抢占** |
| Main 处理完 **Sleep** | 降回 **低 Level 任务** |
| **同 Level** | Round-Robin 于该队列内 |

**鼠标卡顿：** Main **Level 3** + **事件 Wakeup** → **输入延迟 ≈ 处理时间** — **非 (N-1)×20ms**。

---

### 三、与 Ch13 对比

| | Ch13 | Ch14 |
|---|------|------|
| 队列 | 单 **vector** 全体 | **分级 running_** |
| 唤醒 | 无 | **SendMessage + Wakeup** |
| 公平 | 绝对公平 · **伤交互** | **交互优先** · 后台仍跑 |

→ [ULK 进程调度](../../../06-Understanding-Linux-Kernel/chapter-07-process-scheduling.md)

---

← [4. 性能验证](./section-4-性能验证.md) · 下一节 [6. Idle](./section-6-Idle-Task与小结.md)
