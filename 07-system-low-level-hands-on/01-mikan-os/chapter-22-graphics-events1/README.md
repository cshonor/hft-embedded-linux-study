# Ch 22 · 图形和事件（1）

> **原书第 22 章** · HFT **⚪** · 官方源码标签 `osbook_day22`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **GUI 进阶：** **WinFillRectangle · DoWinFunc** · **LAYER_NO_REDRAW** · **ReadEvent**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 规范** | **`exit()`/`atexit`** | 标准 C 退出 |
| **② 绘制** | **矩形/点 · 直线 · libm** | **stars · lines** |
| **③ 性能** | **GetCurrentTick** · **批量 WinRedraw** | **~99× 加速** |
| **④ 交互** | **CloseWindow · ReadEvent** | **Ctrl+Q · 事件驱动** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. exit 规范化与 atexit | [notes/section-2-exit规范化与atexit.md](./notes/section-2-exit规范化与atexit.md) |
| 3. WinFillRectangle 与 DoWinFunc | [notes/section-3-WinFillRectangle与DoWinFunc.md](./notes/section-3-WinFillRectangle与DoWinFunc.md) |
| 4. 性能测量与批量重绘 | [notes/section-4-性能测量与批量重绘.md](./notes/section-4-性能测量与批量重绘.md) |
| 5. WinDrawLine 与 lines 命令 | [notes/section-5-WinDrawLine与lines命令.md](./notes/section-5-WinDrawLine与lines命令.md) |
| 6. CloseWindow、ReadEvent 与小结 | [notes/section-6-CloseWindow-ReadEvent与小结.md](./notes/section-6-CloseWindow-ReadEvent与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **丰富绘制 syscall** · **重绘优化** · **按键事件等待** |
| 与 02 川合 OS 对照？ | 01 **Day 22+ GUI**；Mikan **stars/lines + ReadEvent** |
| 与 Linux / CSAPP 对照？ | **事件循环** 雏形 — GUI 细节 HFT **⚪ 可跳过** |

**本章目的：** **图形表现力 + 事件驱动架构雏形** — 应用 **等键盘再退出**

---

## 本章学习目标 · 自检

- [ ] **`DoWinFunc`** 可变参模板如何 **泛化 Win syscall**
- [ ] **`LAYER_NO_REDRAW`** 高 32 位标志 · **WinRedraw 批量**
- [ ] **`WinDrawLine`** 斜率与 **floor/ceil**
- [ ] **`ReadEvent`** · **kQuit (Ctrl+Q)**

---

## 相关

- 上一章：[../chapter-21-window-apps/](../chapter-21-window-apps/)
- 下一章：[../chapter-23-graphics-events2/](../chapter-23-graphics-events2/)
- 前置：[../chapter-09-layers/](../chapter-09-layers/) · [../chapter-11-timer-acpi/](../chapter-11-timer-acpi/) · [../chapter-21-window-apps/](../chapter-21-window-apps/)
