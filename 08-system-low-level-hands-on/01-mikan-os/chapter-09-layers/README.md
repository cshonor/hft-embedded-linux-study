# Ch 9 · 叠加过程

> **原书第 9 章** · HFT **⚪** · 官方源码标签 `osbook_day09`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **图层合成 + 性能：** `new`/`sbrk` · **LayerManager** · **Shadow Buffer** · **先测量后优化**

---

## 本章定位 · 前后章关系

| | |
|---|---|
| **本章干什么** | **Layer / WindowManager**；多图层 **合成** 与 **Shadow Buffer** 加速。 |
| **全书作用** | **GUI 合成层** — 把多个绘制面叠成桌面；Ch10 窗口、Ch15 终端都依赖图层消息模型。 |
| **← 前置** | [Ch8 内存](../chapter-08-memory/) — 可动态分配绘制缓冲 |
| **→ 后续** | [Ch10 窗口](../chapter-10-window/) — 可拖动窗口与双缓冲 |

---
### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 堆** | **`sbrk`** 桥接 Newlib · **`new`** | Ch8 分配器 → C++ 动态对象 |
| **② 图层** | **Window / Layer / LayerManager** | 栈式合成 · 不再擦坏底图 |
| **③ 测量** | **Local APIC 定时器** | 重绘 ~**2.5 亿** tick 基准 |
| **④ 加速** | **Shadow Buffer + `memcpy`** | 鼠标 **~67×** · 滚动 **~11×** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. sbrk 与 new 运算符 | [notes/section-2-sbrk与new运算符.md](./notes/section-2-sbrk与new运算符.md) |
| 3. Window、Layer 与 LayerManager | [notes/section-3-Window与LayerManager.md](./notes/section-3-Window与LayerManager.md) |
| 4. Local APIC 定时器测量 | [notes/section-4-Local-APIC定时器测量.md](./notes/section-4-Local-APIC定时器测量.md) |
| 5. 阴影缓冲区与 memcpy 加速 | [notes/section-5-阴影缓冲区与memcpy加速.md](./notes/section-5-阴影缓冲区与memcpy加速.md) |
| 6. 控制台滚动优化与小结 | [notes/section-6-控制台滚动优化与小结.md](./notes/section-6-控制台滚动优化与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **多图层合成** + **阴影缓冲** · 解决 Ch6/7 **擦除破坏** 与 **全屏重绘慢** |
| 与 02 川合 OS 对照？ | 01 较少 **GUI 图层**；Mikan 面向 **GOP 像素 GUI** |
| 与 Linux / CSAPP 对照？ | 合成 ≈ **compositor 雏形**；优化 ≈ [03 SysPerf](../../../15-Systems-Performance-2nd/) **度量驱动** |

**本章目的：** 现代 GUI **图层模型** + **「先测量、后优化」** 性能方法论。

---

## 本章学习目标 · 自检

- [ ] 实现 **`sbrk`** 连接 **BitmapMemoryManager** 与 **`malloc`/`new`**
- [ ] 说清 **Layer 栈** 从底到顶绘制顺序
- [ ] 用 **APIC 定时器** 量化重绘耗时
- [ ] 解释 **Shadow Buffer** 与 **`memcpy` ~67×** 提升原因
- [ ] 优化 **Console 滚动**（像素块移动 + 只重绘末行）

---

## 相关

- 上一章：[../chapter-08-memory/](../chapter-08-memory/)
- 下一章：[../chapter-10-window/](../chapter-10-window/)
- 伏笔来源：[../chapter-06-mouse-pci/](../chapter-06-mouse-pci/) · [../chapter-05-console-text/](../chapter-05-console-text/)
- 后续：[../chapter-11-timer-acpi/](../chapter-11-timer-acpi/)
