## 6. 窗口拖动与 draggable 属性

---

### 一、鼠标按键状态

**USB 鼠标 HID 报告** 除 **Δx, Δy** 外含 **按键位**（左/右/中）。

| 改动 | 说明 |
|------|------|
| 驱动 / ISR → **Message** | 携带 **buttons** 状态 |
| 主循环 | 根据 **左键按下** 进入拖窗模式 |

→ [Ch7 Message + FIFO](../chapter-07-interrupt-fifo/notes/section-5-FIFO与ArrayQueue.md)

---

### 二、拖动逻辑

```
左键按下 (mouse down):
    hit = 最顶层 Layer，点 (cursor_x, cursor_y) 在其内
    if hit && hit.draggable:
        drag_target = hit
        grab_offset = cursor - hit.position

鼠标移动 (move):
    if drag_target:
        drag_target.position += posdiff   // 或 = cursor - grab_offset

左键释放:
    drag_target = nullptr
```

| 概念 | 说明 |
|------|------|
| **Hit test** | 从 **layer_stack 顶向下** 找第一个含该点的 Layer |
| **posdiff** | 本次位移 — 与 Ch6 **Δx/Δy** 一致 |

**重绘：** 拖动时 **脏区** = 旧位置 ∪ 新位置 包围盒 — 配合 §4 局部 Draw + §5 Back Buffer。

---

### 三、`draggable_` 属性

**问题：** 未过滤时 **控制台、桌面** 也被拖走。

**修复：** **Layer** 增加 **`draggable_`（bool）**：

| Layer | draggable_ |
|-------|------------|
| 桌面背景 | **false** |
| 控制台 | **false** |
| 主窗口 | **true** |
| 鼠标光标层 | **false**（随指针更新，非拖窗） |

**mousedown 时：** 仅当 **`draggable_ == true`** 才绑定 **drag_target**。

---

### 四、本章总结

| 成果 | 说明 |
|------|------|
| **边界钳制** | 鼠标不穿屏 |
| **GUI 窗口** | 标题栏 · 关闭钮 · 计数器 |
| **局部重绘** | Layer ID / Rect · **`&` 交集** |
| **Back Buffer** | 消除 **层间闪烁** |
| **拖窗** | 按键 + 顶层 hit + **`draggable_`** |

```
Ch9  图层合成
    ↓
Ch10 可交互 GUI 窗口  ← 本章
    ↓
Ch11+ 定时器 · 多任务 · 窗口应用…
```

---

### 五、后续索引

| Ch10 主题 | 继续读 |
|----------|--------|
| 定时器 / ACPI | [chapter-11-timer-acpi](../chapter-11-timer-acpi/) 🔴 |
| 图形事件 | [chapter-22-graphics-events1](../chapter-22-graphics-events1/) ⚪ |
| 窗口应用 | [chapter-21-window-apps](../chapter-21-window-apps/) ⚪ |
| 图层基础 | [chapter-09-layers](../chapter-09-layers/) |

---

← [5. Back Buffer](./section-5-后置缓冲区.md) · [Ch 9](../chapter-09-layers/) · [Ch 10 导读](../README.md)
