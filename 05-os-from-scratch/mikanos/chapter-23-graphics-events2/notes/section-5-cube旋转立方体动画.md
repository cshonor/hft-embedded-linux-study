## 5. cube 旋转立方体动画

---

### 一、动画 = 定时 + 重绘

```
loop:
    更新旋转角 θ
    3D 顶点 → 2D 投影
    清屏 / 画边
    WinRedraw(layer)
    SetTimer(16);          // ~60fps 示意
    ReadEvent(&ev);        // 等 kTimeout 或 kQuit
```

**依赖 Ch23 定时器 **按任务投递**** — 帧率 **可控** · **不 busy-loop**。

---

### 二、cube 渲染（概念）

| 步骤 | 说明 |
|------|------|
| **8 顶点** | 单位立方体 **3D 坐标** |
| **旋转矩阵** | 绕 X/Y **更新 θ** |
| **正交/弱透视投影** | → 2D **屏幕坐标** |
| **WinDrawLine** | 12 条边 · **棱线分色** |

```cpp
for (edge : cube_edges)
    WinDrawLine(layer, p1.x, p1.y, p2.x, p2.y, color);
WinRedraw(layer);
```

**无 3D 硬件** — 纯 **CPU 线框** — 验证 **动画事件环**。

---

### 三、与 Ch22 性能

| 策略 | cube |
|------|------|
| **每帧全 WinRedraw** | 边数少 · **可接受** |
| **stars 式 NO_REDRAW** | 帧内多线段时可 **defer** |

→ [Ch22 批量重绘](../chapter-22-graphics-events1/notes/section-4-性能测量与批量重绘.md)

---

← [4. timer](./section-4-定时器重构与timer命令.md) · 下一节 [6. blocks](./section-6-键盘升级与blocks游戏.md)
