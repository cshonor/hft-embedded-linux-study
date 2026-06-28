## 6. 事件循环与并发控制

---

### 一、主循环架构

```
        ┌─────────── 硬件（xHC USB）──────────┐
        │              MSI 中断                │
        └────────────────┬───────────────────┘
                         ▼
                  ISR: Push(Message)
                         │
                         ▼
                  ┌─────────────┐
                  │ ArrayQueue  │
                  └──────┬──────┘
                         │ Pop
                         ▼
              Main: 更新坐标 · 画光标
```

**事件循环（示意）：**

```cpp
for (;;) {
    Message m;
    if (queue.Pop(m)) {
        ApplyMouseMove(m);
        RedrawCursor();
    } else {
        __asm__("hlt");   // 队列空 — 休眠至下一中断
    }
}
```

| 组件 | 角色 |
|------|------|
| **ISR** | 生产者 — **只 Push** |
| **Main** | 消费者 — **Pop + 重绘** |
| **`hlt`** | 无消息时 **不占 CPU** — 中断唤醒 |

→ [Ch3 hlt](../chapter-03-bootloader-display/notes/section-3-第一个内核与ELF加载.md)

---

### 二、数据冲突与 cli / sti

**竞态：** Main **Pop** 与 ISR **Push** 同时改 **head/tail/count** → 队列损坏。

**保护：** 操作队列时 **短暂关中断**：

```cpp
void PushSafe(const Message& m) {
    __asm__("cli");
    queue.Push(m);
    __asm__("sti");
}
// Main 侧 Pop 同理 cli/sti 包裹
```

| 指令 | 效果 |
|------|------|
| **`cli`** | Clear Interrupt flag — **屏蔽可屏蔽硬件中断** |
| **`sti`** | Set Interrupt flag — **重新允许中断** |

| 注意 | 说明 |
|------|------|
| **临界区要短** | 关中断过久 → **丢事件/增加延迟** |
| **单核模型** | 本章 **无多核并发** — cli 足够；多核需锁（远后） |

→ 对照 [01 Day 12 缩短关中断时间](../../01-30days-os/day-12-timer1/)

---

### 三、本章总结

| 从 | 到 |
|----|-----|
| 轮询死循环 | **中断唤醒** |
| ISR 干重活 | **FIFO + 主循环** |
| 空转 burn CPU | **`hlt` 休眠** |
| 裸共享队列 | **`cli`/`sti` 临界区** |

**为后续铺路：**

- **Ch 11 定时器** — 又一 **Message 源**
- **Ch 13 多任务** — 在 **事件环** 上调度多线程

---

### 四、后续索引

| Ch7 主题 | 继续读 |
|----------|--------|
| 内存管理 | [chapter-08-memory](../chapter-08-memory/) 🔴 |
| 图层（Ch6 擦除问题） | [chapter-09-layers](../chapter-09-layers/) ⚪ |
| 定时器 | [chapter-11-timer-acpi](../chapter-11-timer-acpi/) 🔴 |
| 多任务 | [chapter-13-multitask1](../chapter-13-multitask1/) 🔴 |

---

← [5. FIFO](./section-5-FIFO与ArrayQueue.md) · [Ch 6](../chapter-06-mouse-pci/) · [Ch 7 导读](../README.md)
