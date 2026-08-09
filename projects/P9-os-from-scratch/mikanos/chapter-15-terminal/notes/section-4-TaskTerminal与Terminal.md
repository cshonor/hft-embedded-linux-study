## 4. TaskTerminal 与 Terminal

---

### 一、TaskTerminal 独立任务

**终端从 Main 拆出** — 独立 **`TaskTerminal`**：

| 收益 | 说明 |
|------|------|
| **模块化** | Main 专注 **合成/输入路由** |
| **多终端预告** | Ch24 **多终端** — 每终端一 Task |
| **消息驱动** | 自有 **`msgs_`** — Ch14 模型 |

```
TaskTerminal loop:
    Pop Message → 键盘字符 / kTimerTimeout(caret) / …
    更新 Terminal 状态
    SendMessage(main, kLayer, RedrawTerminal, …)
```

---

### 二、Terminal 类 — 黑底白字

**经典终端外观：**

| 属性 | 值 |
|------|-----|
| **背景** | 黑色填充 |
| **前景** | 白色 **WriteAscii** |
| **缓冲** | 行/列文本状态 |
| **光标** | 竖线 **caret** |

→ 复用 [Ch5 Console/WriteAscii](../chapter-05-console-text/) · [Ch12 退格/光标](../chapter-12-keyboard/)

---

### 三、0.5 秒闪烁光标

**Ch12 机制复用** — **TimerManager** 注册 **500ms 周期定时器**：

```
kTimerTimeout → TaskTerminal
    → caret_visible = !caret_visible
    → 请求 Main kLayer Redraw（小区域 — §5）
```

**区别 Ch12：** 终端在 **独立 Task** · 绘制经 **Main 合成** · **DrawArea 优化**。

---

← [3. 活动窗口](./section-3-ActiveLayer与ToplevelWindow.md) · 下一节 [5. DrawArea](./section-5-DrawArea局部重绘.md)
