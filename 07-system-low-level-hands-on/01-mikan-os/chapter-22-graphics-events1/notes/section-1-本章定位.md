## 1. 本章定位

> **《从零自制操作系统》Ch 22 图形和事件（1）**

---

### 一、从 Ch21 到「能画、能等」

| Ch 21 | **Ch 22** |
|-------|-----------|
| **OpenWindow · WinWriteString** | **点/矩形/直线** · **性能优化** |
| **winhello 即开即关** | **ReadEvent** — **等 Ctrl+Q 再 exit** |
| 基础 syscall 集 | **DoWinFunc** 工程化 · **CloseWindow** |

**跨越：** 静态 **画字** → **动态绘图 + 事件驱动** GUI 雏形。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **`exit()`/`atexit`** | 标准库退出 · 后处理 |
| **WinFillRectangle** | **stars** 随机星点 |
| **LAYER_NO_REDRAW** | 批量 **WinRedraw** · **~99×** |
| **WinDrawLine + libm** | **lines** 放射彩线 |
| **CloseWindow** | **RemoveLayer** · 无窗口残骸 |
| **ReadEvent** | 阻塞等 **键盘事件** |

---

### 三、后续

```
Ch22 绘制 + 键盘事件(1)  ← 本章
    ↓
Ch23 图形和事件(2) — 鼠标等
Ch24+ 更复杂交互
```

→ [Ch21 窗口应用](../chapter-21-window-apps/)

---

← [Ch 22 导读](../README.md) · 下一节 [2. exit/atexit](./section-2-exit规范化与atexit.md)
