## 6. Idle Task 与小结

---

### 一、全 Sleep 的致命 Bug

若 **所有 Task Sleep** → **`running_` 全空** → 调度器 **无下一 Task** → **崩溃/死锁**。

---

### 二、Idle Task 设计

| 属性 | 值 |
|------|-----|
| **Level** | **0（最低）** |
| **行为** | 循环 **`hlt`** — 等中断唤醒 |
| **Sleep** | **永不** — **永远在 running_[0]** |
| **作用** | **队列永非空** · **空闲省电** |

```cpp
void IdleTaskEntry() {
    for (;;) {
        __asm__("hlt");
    }
}
```

| 现代 OS 对照 | Linux **`idle`** 进程 · **`cpu_idle()`** |
|--------------|-------------------------------------------|

→ [Ch7 hlt](../chapter-07-interrupt-fifo/notes/section-6-事件循环与并发控制.md)

---

### 三、系统空闲时的行为

```
Main Sleep · TaskB Sleep（或也 Sleep）
    ↓
Schedule → 仅 Idle 可跑
    ↓
hlt → 中断（鼠标/定时器）→ Wakeup Main → Level 3 抢占
```

**功耗：** 无忙等 **空转** — **中断驱动唤醒**。

---

### 三、本章总结

| 成果 | 说明 |
|------|------|
| **Sleep/Wakeup + running_** | 无事不占 CPU |
| **per-Task msgs_** | **事件驱动** |
| **TaskB +1.4×** | 休眠验证 |
| **Level 0–3** | **鼠标流畅** |
| **Idle hlt** | **安全 + 省电** |

**调度模型：**

```
优先级事件驱动多任务
= Wakeup(高) + 处理 + Sleep + Idle 兜底
```

---

### 四、后续索引

| Ch14 主题 | 继续读 |
|----------|--------|
| 终端 | [chapter-15-terminal](../chapter-15-terminal/) ⚪ |
| 系统调用 | [chapter-20-syscall](../chapter-20-syscall/) 🔴 |
| 分页 | [chapter-19-paging](../chapter-19-paging/) 🔴 |
| Ch13 基础 | [chapter-13-multitask1](../chapter-13-multitask1/) |

---

← [5. 优先级](./section-5-任务优先级Level.md) · [Ch 13](../chapter-13-multitask1/) · [Ch 14 导读](../README.md)
