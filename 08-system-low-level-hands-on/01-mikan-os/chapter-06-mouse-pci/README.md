# Ch 6 · 鼠标输入和 PCI

> **原书第 6 章** · HFT **🟡** · 官方源码标签 `osbook_day06`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **从输出到输入：** USB **xHCI** · **PCI 枚举** · **轮询** 鼠标光标

---

## 本章定位 · 前后章关系

| | |
|---|---|
| **本章干什么** | **PCI 枚举** · **xHCI** · **鼠标轮询**；理解 BAR 与设备寄存器映射。 |
| **全书作用** | **输入与总线** — 从「只看屏幕」到「读硬件事件」；为 Ch7 中断 + FIFO 提供「为何要异步」的动机。 |
| **← 前置** | [Ch5 Console](../chapter-05-console-text/) |
| **→ 后续** | [Ch7 中断 / FIFO](../chapter-07-interrupt-fifo/) — 用中断替代忙等轮询 |

---
### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① UI** | 光标 · 桌面 · 任务栏 | `FillRectangle` / `DrawRectangle` |
| **② 总线** | **PCI** 配置空间 · 找 **xHC** | `in`/`out` · Class Code |
| **③ USB** | **xHCI** · **BAR0 MMIO** · 初始化 | 现代 USB 3.x 主机控制器 |
| **④ 输入** | **轮询** 位移 · 擦除重绘光标 | 能动起来 — 但有效率/图层问题 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 光标与桌面 UI | [notes/section-2-光标与桌面UI.md](./notes/section-2-光标与桌面UI.md) |
| 3. USB 分层与 xHCI | [notes/section-3-USB分层与xHCI.md](./notes/section-3-USB分层与xHCI.md) |
| 4. PCI 配置空间与枚举 | [notes/section-4-PCI配置空间与枚举.md](./notes/section-4-PCI配置空间与枚举.md) |
| 5. BAR0 与 xHC 初始化 | [notes/section-5-BAR0与xHC初始化.md](./notes/section-5-BAR0与xHC初始化.md) |
| 6. 轮询输入与遗留问题 | [notes/section-6-轮询输入与遗留问题.md](./notes/section-6-轮询输入与遗留问题.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **PCI 找 xHC** → **MMIO 初始化** → **轮询** USB 鼠标 · 光标跟随 |
| 与 02 川合 OS 对照？ | 01 多用 **PS/2**；Mikan 直接 **xHCI + PCI** — 更贴近现代 PC |
| 与 Linux / CSAPP 对照？ | PCI/USB 驱动栈极简版 — 对照 [04-Linux-Kernel-Development PCI](../../../04-Linux-Kernel-Development/) · [ULK 设备驱动](../../../05-Understanding-Linux-Kernel/) |

**遗留问题（本章自指）：** 轮询 **费 CPU** · 擦除光标 **破坏底图** → Ch 7 **中断** · Ch 9 **图层**

---

## 本章学习目标 · 自检

- [ ] 绘制 **鼠标光标** 与 **桌面/任务栏**
- [ ] 说清 **USB 驱动分层** 与 **xHCI** 角色
- [ ] 用 **PCI 配置空间** 筛选 xHC（Vendor / Class Code）
- [ ] 读 **BAR0** 得 MMIO 基址并完成 **xHC 初始化**
- [ ] 实现 **轮询** 移动光标；能解释 **两大缺陷**

---

## 相关

- 上一章：[../chapter-05-console-text/](../chapter-05-console-text/)
- 下一章：[../chapter-07-interrupt-fifo/](../chapter-07-interrupt-fifo/) 🔴
- 后续：[../chapter-09-layers/](../chapter-09-layers/) ⚪
- 模块导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
