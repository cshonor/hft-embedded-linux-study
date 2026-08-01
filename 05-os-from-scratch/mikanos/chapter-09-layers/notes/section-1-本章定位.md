## 1. 本章定位

> **《从零自制操作系统》Ch 9 叠加过程**

---

### 一、要解决什么问题

| Ch 6–7 行为 | **问题** |
|-------------|----------|
| 移动鼠标 **用背景色擦旧光标** | **任务栏/桌面** 被永久破坏 |
| 图层合成后 **仍全屏重绘** | **极慢** · **闪烁**（约 **2.5 亿** APIC tick/次） |

**本章目标：**

1. **图层（Layer）** — 光标与背景 **分离存储、按序叠加**
2. **性能** — **先测量** · **Shadow Buffer + memcpy** 批量写帧缓冲

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **`sbrk` / `new`** | Newlib ↔ Ch8 **BitmapMemoryManager** |
| **Window / Layer** | 矩形绘图区 · **Z 序** |
| **LayerManager** | **layer_stack_** 底→顶绘制 |
| **APIC Timer** | 客观 **benchmark** |
| **Shadow Buffer** | 与 FB **同格式** · **memcpy** 块拷贝 |
| **Console 滚动** | 像素上移 + **只重绘最后一行** |

---

### 三、在全书中的位置

```
Ch6  擦除破坏底图（伏笔）
Ch7  中断+FIFO 移动光标
Ch8  可 Allocate 内存
    ↓
Ch9  图层 + 性能优化  ← 本章
    ↓
Ch10 窗口系统扩展
```

→ [Ch6 遗留问题](../chapter-06-mouse-pci/notes/section-6-轮询输入与遗留问题.md)

---

← [Ch 9 导读](../README.md) · 下一节 [2. sbrk/new](./section-2-sbrk与new运算符.md)
