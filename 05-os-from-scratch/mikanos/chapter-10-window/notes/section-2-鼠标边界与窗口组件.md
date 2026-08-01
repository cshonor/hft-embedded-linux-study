## 2. 鼠标边界与窗口组件

---

### 一、限制鼠标移动范围

**Bug：** 光标移出屏幕边缘后从 **对侧穿出**（坐标未钳制）。

**修复：** **`ElementMin(a, b)`** / **`ElementMax(a, b)`** — 将坐标限制在合法区间：

```cpp
cursor_x = ElementMax(0, ElementMin(cursor_x, screen_size.x - 1));
cursor_y = ElementMax(0, ElementMin(cursor_y, screen_size.y - 1));
```

| 效果 | 说明 |
|------|------|
| 贴边停止 | 符合 **物理显示器** 直觉 |
| 拖窗时 | 光标与 **hit test** 坐标一致 |

→ [Ch6 鼠标位移](../chapter-06-mouse-pci/notes/section-6-轮询输入与遗留问题.md)

---

### 二、带组件的窗口

利用 Ch9 **Window + Layer** 绘制 **GUI chrome**：

| 区域 | 实现 |
|------|------|
| **标题栏** | 矩形填色 + 标题文字 |
| **客户区** | 背景 · 计数器文本等 |
| **关闭按钮** | **二维字符数组** 定义形状 — `'@'` / `'.'` 等逐像素绘制 |

**与 Ch5/Ch9 复用：** 点阵定义 ≈ **WriteAscii / 光标形状** 同一套路。

```
┌───────────────────────────×┐  ← 关闭钮（点阵）
│ Title                      │
├────────────────────────────┤
│  counter: 12345            │
│                            │
└────────────────────────────┘
```

---

### 三、Layer 放置

主窗口作为 **Layer** 入栈 — 位于桌面之上、鼠标层之下（或按书 Z 序）。

→ [Ch9 LayerManager](../chapter-09-layers/notes/section-3-Window与LayerManager.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 计数器与闪烁](./section-3-计数器与闪烁问题.md)
