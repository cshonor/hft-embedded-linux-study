## 3. 每任务消息队列与事件驱动

---

### 一、废除全局 main_queue

**Ch7–13：** 单一 **`main_queue`** — 所有 Message 进一处 · 主循环 Pop。

**问题：** 多 Task 后 **无法** 按 Task **定向唤醒**。

**Ch14：** 每个 **`Task` 自带 `msgs_`**（如 `std::deque<Message>`）。

---

### 二、主任务事件循环

```cpp
void MainTask() {
    for (;;) {
        Message m;
        if (task.msgs_.Pop(m)) {
            Handle(m);   // 鼠标、键盘、定时器…
        } else {
            task_manager.Sleep(current_task);  // 队列空 → 休眠
        }
    }
}
```

| 状态 | CPU |
|------|-----|
| **有 Message** | 处理 · **高响应** |
| **无 Message** | **Sleep** — **不占片** |

---

### 三、SendMessage 与 ISR 唤醒

```cpp
void SendMessage(Task& dest, const Message& m) {
    dest.msgs_.Push(m);   // cli 保护
    task_manager.Wakeup(dest);
}
```

**硬件路径：**

```
鼠标中断 → ISR 解析 → SendMessage(main_task, mouse_msg)
    → main_task Wakeup → 高优先级调度立即处理
```

| 对比 Ch13 | Ch14 |
|-----------|------|
| 主 Task **每 20ms 才被轮到** | **有事件即 Wakeup** — **事件驱动** |
| 全局队列 | **per-Task 队列** — 消息 **归属清晰** |

→ [Ch7 FIFO/Message](../chapter-07-interrupt-fifo/notes/section-5-FIFO与ArrayQueue.md)

---

← [2. Sleep](./section-2-Sleep与running队列.md) · 下一节 [4. 性能验证](./section-4-性能验证.md)
