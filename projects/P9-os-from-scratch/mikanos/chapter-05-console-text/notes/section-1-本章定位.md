## 1. 本章定位

> **《从零自制操作系统》Ch 5 文本显示和控制台类**

---

### 一、为何需要文本

| Ch 4 能力 | **Ch 5 补齐** |
|-----------|---------------|
| 任意 **像素 / 颜色** | **可读字符** — 启动日志、调试信息 |
| 图形 demo | **「像 OS」** 的滚动输出 |

**本章价值：** 为 Ch 7 **中断**、Ch 8 **内存管理** 等提供 **printk 调试窗口** — 没有控制台，底层 bug 只能盯寄存器。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **字体** | 位图数组 · 1 bit/像素 · `WriteAscii()` |
| **模块化** | `graphics.cpp` · `font.cpp` · 头文件 |
| **東雲字体** | Python + **objcopy** → 链入 `.o` |
| **Newlib** | **`sprintf`** — 嵌入式 C 库 |
| **Console** | 缓冲 · 光标 · **`\n` 滚动** |
| **`printk()`** | **`va_list`** · **`vsprintf`** — 仿 Linux |

---

### 三、依赖关系

```
Ch4 PixelWriter（像素）
    ↓
Ch5 WriteAscii → Console → printk
    ↓
Ch7+ 中断/内存调试输出
```

→ [Ch4 PixelWriter](../chapter-04-pixel-make/notes/section-4-PixelWriter与vtable.md)

---

← [Ch 5 导读](../README.md) · 下一节 [2. WriteAscii](./section-2-WriteAscii与位图字体.md)
