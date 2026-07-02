## 4. 定时器重构与 timer 命令

---

### 一、旧架构问题

**Ch11–14：** APIC 定时器超时消息 **固定发给 Main 任务**。

| 局限 | 影响 |
|------|------|
| 应用 **无法** `sleep(500ms)` | 只能 **忙等 GetCurrentTick** |
| 动画 **cube** 需 **按任务唤醒** | Main 转发 **不便** |

---

### 二、重构：消息带任务 ID

```cpp
void RequestTimeout(TaskId id, uint64_t after_ms) {
    timer_queue.push({ .fire_tick = now + ms_to_ticks(after_ms),
                       .target_task = id });
}

// 定时器 ISR / Main:
void OnTimerFire(TimerRequest& req) {
    PostMessage(req.target_task, MakeTimeoutMessage());
}
```

| 改进 | 效果 |
|------|------|
| **target_task** | 超时 **直达设定者** |
| **与 Ch14 Sleep** 统一 | **WaitUntil** 可复用 **同一队列** |

→ [Ch11 APIC 定时器](../chapter-11-timer-acpi/) · [Ch14 Sleep/Wakeup](../chapter-14-multitask2/)

---

### 三、timer 命令（CLI）

```
> timer 3000
(等待 3 秒后返回提示符)
```

**实现：** 终端任务 **设定时器 → ReadEvent/Sleep → 超时事件** — 验证 **毫秒级等待** 无需 **占 CPU**。

---

### 四、应用侧 API（概念）

```cpp
// syscall 或 ReadEvent 收 kTimeout
SetTimer(ms);
ReadEvent(&ev);  // ev.type == kTimeout
```

**cube/blocks** 用 **周期性 timer** 驱动 **帧更新**。

---

← [3. paint](./section-3-鼠标按键与paint绘图.md) · 下一节 [5. cube](./section-5-cube旋转立方体动画.md)
