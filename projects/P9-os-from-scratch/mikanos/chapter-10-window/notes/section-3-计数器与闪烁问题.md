## 3. 计数器与闪烁问题

---

### 一、快速计数器演示

为展示 **动态 UI**，主循环中：

| 改动 | 目的 |
|------|------|
| **去掉 `hlt`** | CPU **全速** 跑 — 计数器递增 |
| 每帧更新窗口内 **数字** | 演示「活」的界面 |

```cpp
++counter;
window->DrawCounter(counter);
layer_manager.DrawAll();   // 初版：仍全屏合成
```

→ 与 [Ch7 hlt 休眠](../chapter-07-interrupt-fifo/notes/section-6-事件循环与并发控制.md) **权衡** — 动画要 **频率**，idle 要 **hlt**

---

### 二、暴露的问题：剧烈闪烁

**每次计数值变化 → 触发全屏 `DrawAll()`：**

| 原因 | 现象 |
|------|------|
| 百万像素 **反复 memcpy/合成** | **FPS 低** |
| 用户可见 **整屏刷新** | **剧烈闪烁** |
| 鼠标层与计数器层 **交替全屏上屏** | 即使 Ch9 Shadow 优化仍 **不够** |

**结论：** 动态区域需要 **局部更新** + **双缓冲**（§4、§5）。

---

### 三、优化方向预览

```
全屏 DrawAll（每 counter tick）  ❌
    ↓
只重绘 counter 所在 Layer/矩形   ← §4
    ↓
Back Buffer 合成后一次 blit      ← §5
```

→ [Ch9 APIC 测量](../chapter-09-layers/notes/section-4-Local-APIC定时器测量.md) — 可对比优化前后

---

← [2. 边界与组件](./section-2-鼠标边界与窗口组件.md) · 下一节 [4. 局部重绘](./section-4-局部重绘与矩形交集.md)
