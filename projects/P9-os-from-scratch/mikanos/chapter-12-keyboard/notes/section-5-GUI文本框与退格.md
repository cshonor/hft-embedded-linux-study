## 5. GUI 文本框与退格

---

### 一、文本框组件

在 **Ch10 窗口** 客户区内绘制 **专用 TextBox**：

| 状态 | 说明 |
|------|------|
| **buffer** | 已输入字符序列 |
| **cursor_index** | 下一个插入位置 |
| **Draw()** | 背景框 + **WriteAscii** 逐字 |

**主循环修改：**

```
Pop keyboard Message → char c = Translate(keycode, shift)
    → textbox.Insert(c) → 局部重绘 TextBox Layer
```

→ [Ch10 局部 Draw](../chapter-10-window/notes/section-4-局部重绘与矩形交集.md)

---

### 二、退格键 `\b`

**Backspace keycode** → 逻辑字符 **`'\b'`** 或专用分支：

```
1. 若 buffer 非空：pop 最后一字符
2. 用 **背景色** 覆盖该字像素区域（或整框重绘）
3. cursor_index--
```

| 与 Ch5 Console | 区别 |
|----------------|------|
| Console **Scroll** | 文本框 **单行/区域** 内编辑 |
| 同为 | **WriteAscii + 背景色擦除** |

---

### 三、输入焦点（概念）

本章通常 **单一 TextBox 聚焦** — 后续多窗口时可加 **focus 层**（Ch22+ 事件）。

---

← [4. Shift](./section-4-修改键与Shift映射.md) · 下一节 [6. 闪烁光标](./section-6-闪烁光标与小结.md)
