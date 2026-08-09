## 3. 鼠标按键与 paint 绘图

---

### 一、按键状态：位图 + XOR

**鼠标报告含 buttons 位图** — 需区分 **按下沿 / 松开沿**（非仅当前电平）。

```cpp
uint8_t prev = prev_buttons;
uint8_t curr = new_buttons;
uint8_t pressed  = (~prev) & curr;   // 0→1
uint8_t released = prev & (~curr);   // 1→0
```

| 事件 | 生成 |
|------|------|
| **kMouseButtonDown** | **pressed** 某位 |
| **kMouseButtonUp** | **released** 某位 |

**异或/位运算** — 精准 **边沿检测** — 拖曳 **paint** 依赖 **左键按住期间 Move**。

---

### 二、paint 应用

```cpp
bool drawing = false;
while (ReadEvent(&ev)) {
    switch (ev.type) {
    case kMouseButtonDown:
        if (ev.button == Left) { drawing = true; last = ev.pos; }
        break;
    case kMouseMove:
        if (drawing) {
            WinDrawLine(layer, last.x, last.y, ev.x, ev.y, color);
            last = ev.pos;
        }
        break;
    case kMouseButtonUp:
        drawing = false;
        break;
    }
}
```

| 交互 | 说明 |
|------|------|
| **按住左键拖动** | 连续 **线段** — 简易 **画图板** |
| **NO_REDRAW 可选** | 拖曳时可 **defer** · 松开 **WinRedraw** |

→ [Ch22 WinDrawLine](../chapter-22-graphics-events1/notes/section-5-WinDrawLine与lines命令.md)

---

### 三、活动窗口路由

**按键/移动事件** 只发给 **ActiveLayer 对应任务** — 与 Ch15 **焦点** 一致 — **多窗时** 仅 **前台** 收事件。

---

← [2. eye](./section-2-鼠标移动与eye应用.md) · 下一节 [4. timer](./section-4-定时器重构与timer命令.md)
