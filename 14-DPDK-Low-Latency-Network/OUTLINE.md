# DPDK — 主题目录

> 按实体书梯度组织：**01-Intro-Book** → **02-Advanced-Book**

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 |

---

## 01-Intro-Book · 《深入浅出 DPDK》

| # | 主题 | 笔记 | HFT |
|---|------|------|-----|
| — | 实体书递进 | [note-DPDK实体书递进](./01-Intro-Book/notes/note-DPDK实体书递进.md) | 🟡 |
| 1 | 认识 DPDK | [chapter-01-认识DPDK/](./01-Intro-Book/chapter-01-认识DPDK/) · [stub](./01-Intro-Book/notes/chapter-01-DPDK架构与EAL.md) | 🔴 |
| 2 | Cache 与内存 | [chapter-02-Cache与内存/](./01-Intro-Book/chapter-02-Cache与内存/) · [stub](./01-Intro-Book/notes/chapter-02-Cache与内存.md) | 🔴 |
| 3 | 并行计算 | [chapter-03-并行计算/](./01-Intro-Book/chapter-03-并行计算/) · [stub](./01-Intro-Book/notes/chapter-03-并行计算.md) | 🔴 |
| 4 | 同步互斥机制 | [chapter-04-同步互斥机制/](./01-Intro-Book/chapter-04-同步互斥机制/) · [stub](./01-Intro-Book/notes/chapter-04-同步互斥机制.md) | 🔴 |
| 5 | mbuf、mempool | [chapter-02-mbuf](./01-Intro-Book/notes/chapter-02-mbuf与内存池.md) | 🔴 |
| 6 | PMD、poll mode | [chapter-03-PMD](./01-Intro-Book/notes/chapter-03-PMD与轮询模式.md) | 🔴 |
| 7 | 零拷贝、旁路 | [chapter-04-零拷贝](./01-Intro-Book/notes/chapter-04-零拷贝与用户态旁路.md) | 🔴 |
| 8 | UDP 组播行情 | [chapter-05](./01-Intro-Book/notes/chapter-05-组播行情接入.md) | 🔴 |
| 9 | 流分类与多队列 | [chapter-08-流分类与多队列/](./01-Intro-Book/chapter-08-流分类与多队列/) · [stub](./01-Intro-Book/notes/chapter-08-流分类与多队列.md) | 🔴 |

**code：** [01-Intro-Book/code/mcast-minimal/](./01-Intro-Book/code/mcast-minimal/)

---

## 02-Advanced-Book · 《Linux 高性能网络详解》

| 主题 | 笔记 | HFT |
|------|------|-----|
| OpenOnload / RDMA / RoCE | [note-openonload-rdma对比](./02-Advanced-Book/notes/note-openonload-rdma对比.md) | 🟡 |
| XDP / tc-BPF 对照 | [note-XDP与DPDK对照](./02-Advanced-Book/notes/note-XDP与DPDK对照.md) | 🟡 |

**code：** [02-Advanced-Book/code/](./02-Advanced-Book/code/)（待补充）

---

## 建议阅读顺序

```
09 Rosen
    ↓
01-Intro-Book：① 深入浅出 DPDK ∥ chapter-01–05 + 官方 doc
    ↓
02-Advanced-Book：② Linux 高性能网络详解 ∥ RDMA/XDP notes
    ↓
11 HFT Practice · ch06
```

> **何时开读：** `01`/`02` 打底 + perf 定位网络瓶颈。详见 [note-DPDK实体书递进](./01-Intro-Book/notes/note-DPDK实体书递进.md)。

跨模块对照 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md)
