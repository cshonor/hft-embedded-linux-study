# Ch 25 · 使用应用读取文件

> **原书第 25 章** · HFT **🟡** · 官方源码标签 `osbook_day25`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **用户态 I/O：** **目录树 · fd · OpenFile/ReadFile · Newlib · readfile · grep**

---

## 本章定位 · 前后章关系

| | |
|---|---|
| **本章干什么** | **OpenFile / ReadFile** syscall · **fd** · **grep**；Newlib **read** 接入。 |
| **全书作用** | **用户态文件读**（🟡）— 把 Ch17 FAT 暴露给应用；Unix **read/open** 语义的最小实现。 |
| **← 前置** | [Ch17 文件系统](../chapter-17-filesystem/) + [Ch24 多终端](../chapter-24-multi-terminal/) |
| **→ 后续** | [Ch26 应用写文件](../chapter-26-app-write-file/) |

---
### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① FS** | **FindFile 递归 · ls 路径** | **apps/** 目录 |
| **② 抽象** | **fd · FileDescriptor** | 跨簇 **read** |
| **③ 桥接** | **open/read/sbrk** · **Newlib** | **fopen/fgets** |
| **④ 应用** | **readfile · grep** | **regex** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 目录树与 ls 升级 | [notes/section-2-目录树与ls升级.md](./notes/section-2-目录树与ls升级.md) |
| 3. apps 目录与 APPS_DIR | [notes/section-3-apps目录与APPS_DIR.md](./notes/section-3-apps目录与APPS_DIR.md) |
| 4. 文件描述符与 FileDescriptor | [notes/section-4-文件描述符与FileDescriptor.md](./notes/section-4-文件描述符与FileDescriptor.md) |
| 5. OpenFile、ReadFile 与 Newlib | [notes/section-5-OpenFile-ReadFile与Newlib.md](./notes/section-5-OpenFile-ReadFile与Newlib.md) |
| 6. readfile、grep 与小结 | [notes/section-6-readfile-grep与小结.md](./notes/section-6-readfile-grep与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **应用读文件** — **fd syscall** · **POSIX open/read** · **grep** |
| 与 02 川合 OS 对照？ | 01 **Day 25+ 文件 API**；Mikan **Task.files_ + FAT** |
| 与 Linux / CSAPP 对照？ | **08 TLPI open/read** · **fd 表** — VFS 前奏 |

**本章目的：** 打通 **应用 ↔ 持久化文件** — 不再只有 **图形与 stdin**

---

## 本章学习目标 · 自检

- [ ] **FindFile** 如何解析 **`apps/foo.elf`**
- [ ] **FileDescriptor** 的 **rd_off_ · 簇边界**
- [ ] **newlib** 的 **open/read/sbrk** 路径
- [ ] **readfile** / **grep** 验证链

---

## 相关

- 上一章：[../chapter-24-multi-terminal/](../chapter-24-multi-terminal/)
- 下一章：[../chapter-26-app-write-file/](../chapter-26-app-write-file/)
- 前置：[../chapter-17-filesystem/](../chapter-17-filesystem/) · [../chapter-20-syscall/](../chapter-20-syscall/) · [../chapter-24-multi-terminal/](../chapter-24-multi-terminal/)
