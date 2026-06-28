# Ch 16 · 命令

> **原书第 16 章** · HFT **⚪** · 官方源码标签 `osbook_day16`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **CLI 落地：** **linebuf_** · **`echo`/`clear`/`lspci`** · **历史 ↑↓** · 移除 TaskB

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 输入** | **行缓冲** · `\b` · **`Scroll1`** · **`>`** | 终端可打字 |
| **② 命令** | **echo · clear · lspci** | 空格分词 · Ch6 PCI |
| **③ 历史** | **`deque` 8 条** · ↑↓ | **`int` vs `size_t` 陷阱** |
| **④ 收尾** | **删除 TaskB** | CPU **个位数** · 省电 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 终端按键输入与行缓冲 | [notes/section-2-终端按键输入与行缓冲.md](./notes/section-2-终端按键输入与行缓冲.md) |
| 3. echo 与 clear 命令 | [notes/section-3-echo与clear命令.md](./notes/section-3-echo与clear命令.md) |
| 4. lspci 命令 | [notes/section-4-lspci命令.md](./notes/section-4-lspci命令.md) |
| 5. 命令历史与方向键 | [notes/section-5-命令历史与方向键.md](./notes/section-5-命令历史与方向键.md) |
| 6. 删除 TaskB 与小结 | [notes/section-6-删除TaskB与小结.md](./notes/section-6-删除TaskB与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **可交互 CLI** — 输入 · 三命令 · 历史 · 省电 |
| 与 01 川合 OS 对照？ | 01 **Day 17+ 命令行**；Mikan **GUI 终端内 shell** |
| 与 Linux / CSAPP 对照？ | 极简 **shell 分词** · **`lspci`** — 08 TLPI 用户态 CLI 对照 |

**本章目的：** 从 **「看」到「用」** — MikanOS 终端 **初具现代 OS 雏形**

---

## 本章学习目标 · 自检

- [ ] 说清 **`linebuf_`** · Enter · **`Scroll1`**
- [ ] 实现 **`echo`/`clear`** 分词逻辑
- [ ] **`lspci`** 遍历 **`pci::devices`**
- [ ] 历史 **↑↓** 与 **signed/unsigned 比较坑**

---

## 相关

- 上一章：[../chapter-15-terminal/](../chapter-15-terminal/)
- 下一章：[../chapter-17-filesystem/](../chapter-17-filesystem/)
- 前置：[../chapter-06-mouse-pci/](../chapter-06-mouse-pci/) · [../chapter-12-keyboard/](../chapter-12-keyboard/)
