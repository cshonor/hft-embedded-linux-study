# Ch 21 · 窗口应用

> **原书第 21 章** · HFT **⚪** · 官方源码标签 `osbook_day21`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **应用生态：** **IST 修复** · **`printf`** · **`exit`** · **`OpenWindow`/`WinWriteString`** · **winhello**

---

## 本章定位 · 前后章关系

| | |
|---|---|
| **本章干什么** | **winhello** 等 **用户态窗口应用** · **syscall.h** 窗口 API。 |
| **全书作用** | **用户态 GUI** — 把 Ch9–10 窗口机制延伸到 Ring 3；Ch22–23 图形事件的应用侧实践。 |
| **← 前置** | [Ch20 syscall](../chapter-20-syscall/) + [Ch10 窗口](../chapter-10-window/) |
| **→ 后续** | [Ch22 图形和事件（1）](../chapter-22-graphics-events1/) |

---
### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 稳定** | **TSS IST1** · 定时器 **专用内核栈** | syscall 窗口内 **#PF 修复** |
| **② 终端 I/O** | **PutString** · **`write()`** | **`printf` 打通** |
| **③ 生命周期** | **`exit` `0x80000002`** | **CallApp 栈恢复** |
| **④ GUI** | **OpenWindow · WinWriteString** | **winhello** 三色三行 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. IST 与定时器中断栈修复 | [notes/section-2-IST与定时器中断栈修复.md](./notes/section-2-IST与定时器中断栈修复.md) |
| 3. PutString 与 printf 适配 | [notes/section-3-PutString与printf适配.md](./notes/section-3-PutString与printf适配.md) |
| 4. exit 系统调用与 CallApp 栈恢复 | [notes/section-4-exit系统调用与CallApp栈恢复.md](./notes/section-4-exit系统调用与CallApp栈恢复.md) |
| 5. syscall.h 与窗口系统调用 | [notes/section-5-syscall.h与窗口系统调用.md](./notes/section-5-syscall.h与窗口系统调用.md) |
| 6. winhello 与小结 | [notes/section-6-winhello与小结.md](./notes/section-6-winhello与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **丰富 syscall** — 终端 **printf** · **exit** · **独立窗口绘制** |
| 与 02 川合 OS 对照？ | 01 **Day 21+ GUI 应用**；Mikan **Layer syscall + winhello** |
| 与 Linux / CSAPP 对照？ | **write/exit** 语义 · GUI 为 **桌面 OS 专有**（HFT 可跳过细节） |

**本章目的：** 应用 **标准库输出 · 正常退出 · 自建 GUI 窗口** — 生态转折点

---

## 本章学习目标 · 自检

- [ ] **IST1** 如何解决 **syscall 期间定时器 #PF**
- [ ] **PutString** 如何用 **任务 ID** 找终端
- [ ] **`exit`** 为何不 **sysret** · **CallApp 保存的 RSP**
- [ ] **OpenWindow → layer ID → WinWriteString**

---

## 相关

- 上一章：[../chapter-20-syscall/](../chapter-20-syscall/)
- 下一章：[../chapter-22-graphics-events1/](../chapter-22-graphics-events1/)
- 前置：[../chapter-09-layers/](../chapter-09-layers/) · [../chapter-15-terminal/](../chapter-15-terminal/) · [../chapter-20-syscall/](../chapter-20-syscall/)
