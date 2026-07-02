## 2. 鼠标移动与 eye 应用

---

### 一、鼠标移动 → 事件

**Ch6 鼠标 polling/中断** 升级：**位移包** 不仅 **移动光标** — 还 **投递给活动窗口任务**。

```
MouseHandler(delta_x, delta_y, buttons):
    UpdateGlobalCursor();
    if (ActiveLayer* layer = GetActiveWindow()) {
        PostEvent(layer->task_id, {
            .type = kMouseMove,
            .pos = cursor_in_window,
            .delta = {dx, dy}
        });
    }
```

| 字段 | 用途 |
|------|------|
| **窗口内坐标** | 绘制 **eye 瞳孔位置** |
| **delta** | 可选 **相对移动** 逻辑 |

→ [Ch6 鼠标](../chapter-06-mouse-pci/) · [Ch15 ActiveLayer](../chapter-15-terminal/)

---

### 二、ReadEvent 扩展

**Ch22 `Event` 结构增加 `kMouseMove`** — 应用 **阻塞 ReadEvent** 收 **移动**。

---

### 三、eye 应用（类 xeyes）

```cpp
while (true) {
    ReadEvent(&ev);
    if (ev.type == kMouseMove) {
        double angle = atan2(ev.y - cy, ev.x - cx);
        DrawPupil(cos(angle)*r, sin(angle)*r);  // 正方形“眼球”跟光标
        WinRedraw(layer);
    }
}
```

| 要点 | 说明 |
|------|------|
| **atan2 / sin / cos** | Ch22 **libm** 延续 |
| **实时重绘** | 每次移动 **更新瞳孔** — 验证 **高频事件** |

**参考：** Linux 经典 **`xeyes`** — 双球 **凝视指针**；本书 **eye** 为 **简化单窗版**。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. paint](./section-3-鼠标按键与paint绘图.md)
