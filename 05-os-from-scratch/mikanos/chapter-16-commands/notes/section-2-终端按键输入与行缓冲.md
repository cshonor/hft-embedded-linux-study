## 2. 终端按键输入与行缓冲

---

### 一、从 TextBox 到 Terminal

**Ch12：** 键盘 → **固定 TextBox**。

**Ch15–16：** 仅 **活动 Terminal**（ToplevelWindow 内）接收按键 → **`TaskTerminal` / `Terminal` 类**。

---

### 二、linebuf_ 行缓冲区

```cpp
class Terminal {
    char linebuf_[MAX_LINE];
    int line_len_;
    // cursor row/col …
};
```

| 按键 | 行为 |
|------|------|
| **可打印字符** | append **linebuf_** · 白字 Draw |
| **Enter** | 解析 **linebuf_** 为命令 · 清空 · 新 **`>`** |
| **`\b` Backspace** | 删末字符 · 背景色覆盖像素 |

→ [Ch12 退格](../chapter-12-keyboard/notes/section-5-GUI文本框与退格.md)

---

### 三、Scroll1() — 触底滚动

输入到达 **终端最底行** 时：

```
Scroll1():
    阴影缓冲/客户区 像素上移一行
    清空最底行
    cursor 行 --
```

**与 Ch15 DrawArea 配合** — 滚动可 **整客户区** 或 **优化块移动**（见 Ch9 Console 滚动思路）。

---

### 四、提示符 `>`

每行开头绘制 **`>`** — 提示 **可输入** · 区分 **命令输出**。

```
> echo hello
hello
> 
```

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. echo/clear](./section-3-echo与clear命令.md)
