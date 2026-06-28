## 3. Window、Layer 与 LayerManager

---

### 一、Window 类

**Window** — 抽象 **有宽高的矩形绘图区域**：

| 属性 | 说明 |
|------|------|
| **width / height** | 像素尺寸 |
| **像素内容** | 最初直接写 FB；后改为 **Shadow Buffer**（§5） |
| **Draw 方法** | 在 **窗口本地坐标** 绘图 |

**示例层：** 桌面背景 Window · 任务栏 Window · 控制台 Window · **鼠标光标 Window**

---

### 二、Layer 类

**Layer** — 窗口在 **屏幕上的平面位置 + 重叠顺序（Z-order）**：

| 属性 | 说明 |
|------|------|
| **position (x, y)** | 层左上角在屏幕坐标 |
| **window*** | 指向 **Window** 内容 |
| **Z 序** | 由 **layer_stack_ 中的顺序** 决定 |

---

### 三、LayerManager 与图层栈

**LayerManager** 维护 **`layer_stack_`** — 从 **底到顶** 的 Layer 列表：

```
DrawAll():
    for layer in layer_stack_ (bottom → top):
        blit layer.window to screen at layer.position
```

| 顺序（示意） | 层 |
|--------------|-----|
| **底** | 蓝色桌面 |
| ↑ | 任务栏 |
| ↑ | 控制台 |
| **顶** | 鼠标光标 |

**鼠标移动时：**

1. 只更新 **顶层光标 Window** 的位置/内容
2. **重新合成** 全屏（初版慢 — §4/§5 优化）
3. **不再** 用「背景色擦除」破坏任务栏 — 底层像素保存在 **各自 Window**

→ 解决 [Ch6 擦除破坏](../chapter-06-mouse-pci/notes/section-6-轮询输入与遗留问题.md)

---

### 四、与 Ch10 关系

**Ch9** 建立 **Layer + 合成** 模型 — **Ch10** 扩展为 **完整窗口系统**（移动、标题栏等）。

→ [chapter-10-window](../chapter-10-window/)

---

← [2. sbrk/new](./section-2-sbrk与new运算符.md) · 下一节 [4. APIC 测量](./section-4-Local-APIC定时器测量.md)
