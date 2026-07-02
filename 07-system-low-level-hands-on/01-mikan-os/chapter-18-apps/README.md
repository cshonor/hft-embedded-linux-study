# Ch 18 · 应用

> **原书第 18 章** · HFT **⚪** · 官方源码标签 `osbook_day18`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **里程碑：** **FAT 簇链 · `cat`** · **磁盘执行** · **ELF · argc/argv** · **C++ RPN**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 读文件** | **簇链** · **`cat`** | Ch17 **ls** → **读内容** |
| **② 执行** | **无头 .bin** · 非内置命令 → **跳内存** | 第一个 **外部程序** |
| **③ 稳定** | 运行前 **`sti`** | **hlt** 不 **假死全系统** |
| **④ 工程化** | **ELF** · **`MakeArgVector`** · **libc++** | **RPN 计算器** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. FAT 簇链与 cat 命令 | [notes/section-2-FAT簇链与cat命令.md](./notes/section-2-FAT簇链与cat命令.md) |
| 3. 无头二进制与磁盘执行 | [notes/section-3-无头二进制与磁盘执行.md](./notes/section-3-无头二进制与磁盘执行.md) |
| 4. sti 与 hlt 冻结 Bug | [notes/section-4-sti与hlt冻结Bug.md](./notes/section-4-sti与hlt冻结Bug.md) |
| 5. ELF 格式与命令行参数 | [notes/section-5-ELF格式与命令行参数.md](./notes/section-5-ELF格式与命令行参数.md) |
| 6. C++ 应用、标准库与小结 | [notes/section-6-C++应用标准库与小结.md](./notes/section-6-C++应用标准库与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **读文件 · 跑外部程序** — 裸 bin / **ELF C++** · **退出码** |
| 与 02 川合 OS 对照？ | 01 **Day 18+ 用户程序**；Mikan **终端 fallback 执行** |
| 与 Linux / CSAPP 对照？ | **exec 前奏** — Ch3 **ELF** · CSAPP **链接与加载** |

**本章目的：** 从 **内置功能** → **独立应用生态** — 加载 · 参数 · 返回

---

## 本章学习目标 · 自检

- [ ] 用 **FAT 表** 走 **簇链** 读完整文件
- [ ] 说清 **非内置命令 → 读盘 → 跳转** 流程
- [ ] 解释 **缺 sti + hlt** 为何 **冻住全 OS**
- [ ] **ELF 魔数** · **`MakeArgVector`** · **exit code**

---

## 相关

- 上一章：[../chapter-17-filesystem/](../chapter-17-filesystem/)
- 下一章：[../chapter-19-paging/](../chapter-19-paging/)
- 前置：[../chapter-03-bootloader-display/](../chapter-03-bootloader-display/) · [../chapter-07-interrupt-fifo/](../chapter-07-interrupt-fifo/) · [../chapter-16-commands/](../chapter-16-commands/)
