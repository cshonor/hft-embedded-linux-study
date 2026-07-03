# Ch 24 · 多终端

> **原书第 24 章** · HFT **⚪** · 官方源码标签 `osbook_day24`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **并发飞跃：** **F2 多终端 · 每应用 PML4 · noterm · KillApp**

---

## 本章定位 · 前后章关系

| | |
|---|---|
| **本章干什么** | **多 Terminal Task** · **每进程 PML4** · **KillApp** · 用户态异常处理。 |
| **全书作用** | **多进程壳** — 从单应用分页到「多个用户程序同时活」；Ch25–29 文件/IPC 的并发场景。 |
| **← 前置** | [Ch19–20 分页与 syscall](../chapter-19-paging/) + [Ch15 终端](../chapter-15-terminal/) |
| **→ 后续** | [Ch25 应用读文件](../chapter-25-app-read-file/) |

---
### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 多终端** | **F2** · **光标/kWindowActive** | 并行 **多应用** |
| **② 隔离** | **专属 PML4 · CR3 切换** | 同 VA **不同 PA** |
| **③ 体验** | **层级 Bug · noterm** | 清爽 **全屏 GUI** |
| **④ 健壮** | **CPL=3 异常 → KillApp** | OS **不因应用崩** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. F2 新终端与光标独立 | [notes/section-2-F2新终端与光标独立.md](./notes/section-2-F2新终端与光标独立.md) |
| 3. 每应用 PML4 与 CR3 切换 | [notes/section-3-每应用PML4与CR3切换.md](./notes/section-3-每应用PML4与CR3切换.md) |
| 4. 窗口层级 Bug 与 noterm | [notes/section-4-窗口层级Bug与noterm.md](./notes/section-4-窗口层级Bug与noterm.md) |
| 5. 用户态异常与 KillApp | [notes/section-5-用户态异常与KillApp.md](./notes/section-5-用户态异常与KillApp.md) |
| 6. 小结与后续 | [notes/section-6-小结与后续.md](./notes/section-6-小结与后续.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **多终端并发** · **per-app 页表** · **noterm** · **杀崩应用** |
| 与 02 川合 OS 对照？ | 01 **多任务 shell**；Mikan **GUI 多 Terminal + CR3** |
| 与 Linux / CSAPP 对照？ | **进程地址空间** — Ch19 深化 · **信号/杀进程** 极简版 |

**本章目的：** **多应用同时跑** 且 **应用崩溃不拖死内核**

---

## 本章学习目标 · 自检

- [ ] **F2** 如何 spawn **新 TaskTerminal**
- [ ] 为何 **同链接基址** 需 **不同 PML4**
- [ ] **noterm** 的 **show_window** 与 **自动喂命令**
- [ ] **CPL==3** 异常路径 **KillApp/ExitApp**

---

## 相关

- 上一章：[../chapter-23-graphics-events2/](../chapter-23-graphics-events2/)
- 下一章：[../chapter-25-app-read-file/](../chapter-25-app-read-file/)
- 前置：[../chapter-15-terminal/](../chapter-15-terminal/) · [../chapter-19-paging/](../chapter-19-paging/) · [../chapter-20-syscall/](../chapter-20-syscall/)
