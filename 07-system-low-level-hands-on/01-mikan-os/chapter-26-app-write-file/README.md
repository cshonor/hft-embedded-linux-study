# Ch 26 · 使用应用写入文件

> **原书第 26 章** · HFT **🟡** · 官方源码标签 `osbook_day26`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **统一 I/O：** **FileDescriptor 继承 · stdin/stdout · O_CREAT · Write · cp**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 抽象** | **基类 fd · Terminal / FAT 派生** | 键盘 ≡ **字节流** |
| **② stdin** | **fd=0 · Echo · @stdin · Ctrl+D** | **EOF/EOT** |
| **③ 写盘** | **O_CREAT · ExtendCluster · Write** | **AllocateClusterChain** |
| **④ 集成** | **stdout/stderr · cp** | **stdio 完整环** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. FileDescriptor 继承与终端 fd | [notes/section-2-FileDescriptor继承与终端fd.md](./notes/section-2-FileDescriptor继承与终端fd.md) |
| 3. stdin、回显与 Ctrl+D | [notes/section-3-stdin回显与Ctrl+D.md](./notes/section-3-stdin回显与Ctrl+D.md) |
| 4. O_CREAT 与 FAT 写扩展 | [notes/section-4-O_CREAT与FAT写扩展.md](./notes/section-4-O_CREAT与FAT写扩展.md) |
| 5. Write 与标准输出 | [notes/section-5-Write与标准输出.md](./notes/section-5-Write与标准输出.md) |
| 6. cp 命令与小结 | [notes/section-6-cp命令与小结.md](./notes/section-6-cp命令与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **写文件 + stdin/stdout** — **fd 统一** · **cp** |
| 与 02 川合 OS 对照？ | 01 **Day 26+ 写盘**；Mikan **O_CREAT + TerminalFD** |
| 与 Linux / CSAPP 对照？ | **08 TLPI fd 0/1/2** · **everything is a file** 极简版 |

**本章目的：** 键盘 · 终端 · 磁盘 **同一 fd 抽象** — I/O 架构 **规范化**

---

## 本章学习目标 · 自检

- [ ] **TerminalFileDescriptor** vs **fat::FileDescriptor**
- [ ] **Ctrl+D (EOT)** 如何 **模拟 EOF**
- [ ] **O_CREAT** · **ExtendCluster** · **Write 跨簇**
- [ ] **cp** 读写循环

---

## 相关

- 上一章：[../chapter-25-app-read-file/](../chapter-25-app-read-file/)
- 下一章：[../chapter-27-app-memory/](../chapter-27-app-memory/)
- 前置：[../chapter-25-app-read-file/](../chapter-25-app-read-file/) · [../chapter-16-commands/](../chapter-16-commands/) · [../chapter-17-filesystem/](../chapter-17-filesystem/)
