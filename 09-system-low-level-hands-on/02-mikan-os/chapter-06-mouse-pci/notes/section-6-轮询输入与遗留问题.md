## 6. 轮询输入与遗留问题

---

### 一、轮询（Polling）获取鼠标位移

控制器就绪后，**主循环** 中反复询问硬件：

```
while (true) {
    if (xHC_has_event()) {
        report = parse_mouse_report();
        cursor_x += report.dx;
        cursor_y += report.dy;
        erase_cursor(old_x, old_y);   // 背景色覆盖
        draw_cursor(cursor_x, cursor_y);
    }
    // 无事件也持续转 — 占满 CPU
}
```

| 轮询 | 说明 |
|------|------|
| **优点** | 实现简单 · **无中断框架** 即可动 |
| **缺点** | **空转浪费 CPU** · 延迟取决于循环频率 |

→ **Ch 7** 改为 **中断 + FIFO** — 有事件才处理

---

### 二、擦除重绘的问题

| 做法 | 缺陷 |
|------|------|
| 旧位置 **涂桌面背景色** | 光标曾经过 **任务栏** → 任务栏被 **抹成蓝色** |
| 未保存 **光标下原始像素** | 复杂背景 **无法恢复** |

**根因：** 帧缓冲 **单层** — 光标与桌面 **同一 bitmap**，无 **图层合成**。

→ **Ch 9 叠加（Layers）** — 光标独立层 · 合成到屏幕

---

### 三、本章成果与债务

| ✅ 完成 | ⚠️ 遗留 |
|---------|---------|
| PCI 找 xHC · MMIO 初始化 | CPU **100% 轮询** |
| USB 鼠标 **位移** 驱动 | **破坏底图** |
| 光标 + 桌面 **视觉** | 无 **按键/点击** 语义（后续扩展） |

```
Ch6 轮询鼠标（本章）
    ↓
Ch7 中断驱动输入
    ↓
Ch9 图层避免擦除破坏
    ↓
Ch10+ 窗口与事件
```

---

### 四、后续索引

| Ch6 主题 | 继续读 |
|----------|--------|
| 中断 / FIFO | [chapter-07-interrupt-fifo](../chapter-07-interrupt-fifo/) 🔴 |
| 图层 | [chapter-09-layers](../chapter-09-layers/) ⚪ |
| 窗口 | [chapter-10-window](../chapter-10-window/) ⚪ |
| printk 调试 | [chapter-05-console-text](../chapter-05-console-text/) |

---

← [5. BAR0 / xHC](./section-5-BAR0与xHC初始化.md) · [Ch 5](../chapter-05-console-text/) · [Ch 6 导读](../README.md)
