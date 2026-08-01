## 4. 抢占式多任务与时间片

---

### 一、抢占式（Preemptive）多任务

**OS 强制切换** — Task **无需配合**：

```
Task A 运行中 …
    ↓ 20ms 定时器中断
ISR → schedule() → SwitchContext(A → B)
    ↓
Task B 运行 …
```

| 对比合作式 | 抢占式 |
|------------|--------|
| Task 自愿 yield | **定时器** 打断 |
| 一 Task 占死 CPU | **时间片** 公平（雏形） |

---

### 二、20ms 时间片

利用 **Ch11/Ch12 校准后的 Local APIC 定时器**：

| 参数 | 值 |
|------|-----|
| **周期** | **0.02s = 20ms** |
| **触发** | 定时器 ISR |
| **ISR 内** | `TaskManager::Switch()` — 轮转 **current_task_** |

**现代 OS：** 通常 **1–10ms** 量级 — 20ms 教学清晰、切换开销可感知。

→ [Ch11 APIC Timer](../chapter-11-timer-acpi/notes/section-3-APIC定时器与TimerManager.md)

---

### 三、切换与中断安全

| 注意 | 说明 |
|------|------|
| **在 ISR 或 ISR 尾调度** | 需 **保存完整中断帧** 或 **专门 reschedule 点** |
| **cli 期间** | 切换逻辑常与 **关中断** 配合 — 防嵌套竞态 |
| **current_task 指针** | **volatile** / 原子 — ISR 与 idle 均访问 |

→ [Ch7 cli/sti](../chapter-07-interrupt-fifo/notes/section-6-事件循环与并发控制.md)

---

← [3. SwitchContext](./section-3-SwitchContext与合作式切换.md) · 下一节 [5. TaskManager](./section-5-Task与TaskManager.md)
