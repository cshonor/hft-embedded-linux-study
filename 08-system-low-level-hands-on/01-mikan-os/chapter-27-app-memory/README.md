# Ch 27 · 应用的内存管理

> **原书第 27 章** · HFT **🔴** · 官方源码标签 `osbook_day27`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **高级内存：** **Demand Paging · MapFile · CoW · memstat · invlpg**

---

## 本章定位 · 前后章关系

| | |
|---|---|
| **本章干什么** | **Demand Paging** · **sbrk** · **CoW** · **invlpg** · **memstat**。 |
| **全书作用** | **进程内存管理**（🔴）— Linux `mmap`/堆/brk 的极简版；理解 **按需分页** 与 **写时复制**。 |
| **← 前置** | [Ch19 分页](../chapter-19-paging/) + [Ch26 stdio](../chapter-26-app-write-file/) |
| **→ 后续** | [Ch28 日文 / 重定向](../chapter-28-japanese-redirect/) |

---
### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 按需** | **#PF → HandlePageFault** · **DemandPages** | **malloc/sbrk** 高效 |
| **② 映射** | **MapFile** · Page Cache | **文件随机访问** |
| **③ 观测** | **memstat** | 物理帧 **已用/总量** |
| **④ 共享** | **CoW · invlpg** | 多 **cube** 省内存 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 按需分页与 DemandPages | [notes/section-2-按需分页与DemandPages.md](./notes/section-2-按需分页与DemandPages.md) |
| 3. MapFile 与内存映射文件 | [notes/section-3-MapFile与内存映射文件.md](./notes/section-3-MapFile与内存映射文件.md) |
| 4. memstat 与位图统计 | [notes/section-4-memstat与位图统计.md](./notes/section-4-memstat与位图统计.md) |
| 5. 写入时复制与 invlpg | [notes/section-5-写入时复制与invlpg.md](./notes/section-5-写入时复制与invlpg.md) |
| 6. 小结与后续 | [notes/section-6-小结与后续.md](./notes/section-6-小结与后续.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **#PF 分配** · **mmap 读文件** · **CoW 共享代码** · **memstat** |
| 与 02 川合 OS 对照？ | 01 **Day 27+ 高级内存**；Mikan **DemandPages + CoW** |
| 与 Linux / CSAPP 对照？ | **CSAPP Ch9** · **08 TLPI mmap/brk** · **LKD CoW** |

**本章目的：** 用 **Page Fault 做延迟分配与共享** — 大 VA 空间 **用得省、用得巧**

---

## 本章学习目标 · 自检

- [ ] **Demand Paging** 与 **Eager 映射** 区别
- [ ] **HandlePageFault** 分配帧 · 填 PTE
- [ ] **MapFile** 故障时 **Page Cache**
- [ ] **CoW** 流程与 **`invlpg`**

---

## 相关

- 上一章：[../chapter-26-app-write-file/](../chapter-26-app-write-file/)
- 下一章：[../chapter-28-japanese-redirect/](../chapter-28-japanese-redirect/)
- 前置：[../chapter-08-memory/](../chapter-08-memory/) · [../chapter-19-paging/](../chapter-19-paging/) · [../chapter-24-multi-terminal/](../chapter-24-multi-terminal/)
