# Ch 10 · 窗口

> **原书第 10 章** · HFT **⚪** · 官方源码标签 `osbook_day10`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **GUI 雏形：** 标题栏窗口 · **局部重绘** · **Back Buffer** · **拖动**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 边界** | `ElementMin/Max` · 窗口组件 | 鼠标不穿屏 · 标题栏/关闭钮 |
| **② 闪烁** | 全速计数器 · 全屏重绘问题 | 动态 UI 的性能代价 |
| **③ 优化** | **局部 Draw** · 矩形 **`&`** | 只更新脏区 |
| **④ 合成** | **Back Buffer** | 双缓冲消闪烁 |
| **⑤ 交互** | 按键状态 · **`draggable_`** | 拖动顶层窗口 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 鼠标边界与窗口组件 | [notes/section-2-鼠标边界与窗口组件.md](./notes/section-2-鼠标边界与窗口组件.md) |
| 3. 计数器与闪烁问题 | [notes/section-3-计数器与闪烁问题.md](./notes/section-3-计数器与闪烁问题.md) |
| 4. 局部重绘与矩形交集 | [notes/section-4-局部重绘与矩形交集.md](./notes/section-4-局部重绘与矩形交集.md) |
| 5. 后置缓冲区 | [notes/section-5-后置缓冲区.md](./notes/section-5-后置缓冲区.md) |
| 6. 窗口拖动与 draggable | [notes/section-6-窗口拖动与draggable.md](./notes/section-6-窗口拖动与draggable.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **GUI 窗口** + **局部/双缓冲** + **鼠标拖窗** |
| 与 02 川合 OS 对照？ | 01 **窗口/图层** 较简；Mikan **Back Buffer + draggable** |
| 与 Linux / CSAPP 对照？ | 双缓冲 ≈ **X11/compositor** 基础；脏矩形 ≈ **damage region** |

**本章目的：** Ch9 图层之上 — **可交互、不闪烁** 的窗口体验。

---

## 本章学习目标 · 自检

- [ ] 用 **`ElementMin/Max`** 钳制鼠标于 **`screen_size`**
- [ ] 绘制 **标题栏 / 关闭按钮**（字符点阵）
- [ ] 说清 **全屏重绘闪烁** 与 **局部 Draw** 优化
- [ ] 解释 **Back Buffer** 一次性 blit 到 FB
- [ ] 实现 **左键拖窗** + **`draggable_`** 过滤

---

## 相关

- 上一章：[../chapter-09-layers/](../chapter-09-layers/)
- 下一章：[../chapter-11-timer-acpi/](../chapter-11-timer-acpi/)
- 前置：[../chapter-06-mouse-pci/](../chapter-06-mouse-pci/) · [../chapter-07-interrupt-fifo/](../chapter-07-interrupt-fifo/)
