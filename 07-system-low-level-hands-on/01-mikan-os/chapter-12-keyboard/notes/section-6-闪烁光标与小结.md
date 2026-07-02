## 6. 闪烁光标与小结

---

### 一、竖线输入光标

在 **当前插入位置末尾** 绘制 **竖线**（`|` 或 1px 宽矩形）— 提示 **可输入**。

**静态光标** 不够 — 用户不确定系统是否响应。

---

### 二、0.5 秒周期定时器

利用 **Ch11 TimerManager** 注册 **周期性定时器**：

| 参数 | 值 |
|------|-----|
| **周期** | **500ms**（0.5s） |
| **超时消息** | **`Message::kTimerTimeout`**（timer_id 区分光标） |

**处理逻辑：**

```
on TimerTimeout(caret_timer_id):
    caret_visible = !caret_visible
    if caret_visible: DrawCaretLine()
    else:             EraseCaretLine()  // 背景色
    Redraw TextBox region
```

| 效果 | 说明 |
|------|------|
| **闪烁** | 业界标准 **text caret** UX |
| **与 Ch11 衔接** | **priority_queue** 多定时器之一 |

→ [Ch11 多定时器](../chapter-11-timer-acpi/notes/section-4-优先级队列与多定时器.md)

---

### 三、本章总结

| 成果 | 说明 |
|------|------|
| **FADT + PM 校准** | **10ms** APIC 周期 |
| **USB HID 键盘** | keycode → ASCII |
| **Shift** | 双映射表 |
| **TextBox + Backspace** | GUI 输入 |
| **Caret blink** | **0.5s** 定时器 |

```
Ch6  USB 栈
Ch11 定时器
    ↓
Ch12 键盘 + 文本框 + 闪烁光标  ← 本章
    ↓
Ch13 多任务
```

---

### 四、后续索引

| Ch12 主题 | 继续读 |
|----------|--------|
| 多任务 | [chapter-13-multitask1](../chapter-13-multitask1/) 🔴 |
| 终端/命令 | [chapter-15-terminal](../chapter-15-terminal/) · [chapter-16-commands](../chapter-16-commands/) |
| 01 键盘 | [01 Day 14 keyboard](../../02-30days-os/day-14-keyboard/) |
| 图形事件 | [chapter-22-graphics-events1](../chapter-22-graphics-events1/) |

---

← [5. 文本框](./section-5-GUI文本框与退格.md) · [Ch 11](../chapter-11-timer-acpi/) · [Ch 12 导读](../README.md)
