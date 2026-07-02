## 4. 优先级队列与多定时器

---

### 一、需求：多路逻辑定时器

| 场景 | 需要 |
|------|------|
| **光标闪烁** | 500ms 周期 |
| **应用 sleep** | 指定 ms 后唤醒 |
| **多个并发** | 同时跟踪 **多组 deadline** |

单一 **`tick_`** 不够 — 需要 **Timer 对象 + 超时调度**。

---

### 二、std::priority_queue

**逻辑定时器** 按 **超时时刻** 排序 — **最近超时者优先**：

```cpp
struct Timer {
    uint64_t timeout_tick;
    int id;
    bool operator<(const Timer& rhs) const {
        return timeout_tick > rhs.timeout_tick;  // min-heap
    }
};

std::priority_queue<Timer> timers_;
```

| 操作 | 说明 |
|------|------|
| **AddTimer(deadline)** | `push` |
| **每 tick / ISR 后** | 若 `top().timeout <= now` → **pop** 并通知 |

**重载 `operator<`：** 使 **timeout 越小堆顶** — 标准库 **大顶堆** 技巧。

---

### 三、Message::kTimerTimeout

超时 **不直接** 在 ISR 里做复杂逻辑 — 与 Ch7 一致，**Push Message**：

```
Timer ISR / Tick 处理:
    while 堆顶已超时:
        Pop timer
        message_queue.Push({ kTimerTimeout, timer_id, ... })
```

**主循环 / 事件环：**

```
Pop Message → case kTimerTimeout: 光标翻转 / 唤醒任务 / …
```

→ [Ch7 FIFO + Message](../chapter-07-interrupt-fifo/notes/section-6-事件循环与并发控制.md)

---

← [3. APIC Timer](./section-3-APIC定时器与TimerManager.md) · 下一节 [5. ACPI 校准](./section-5-ACPI-PM定时器校准.md)
