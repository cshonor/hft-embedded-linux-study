# 01-Intro-Book · 《深入浅出 DPDK》

> **10-DPDK** 二级目录 · **梯度 ①** — 建立 DPDK 旁路认知  
> 实体书 + 官方 Programmer's Guide；与 [02-Advanced-Book](../02-Advanced-Book/) 递进。

---

## 目录结构

```
01-Intro-Book/
├── notes/     ← 章节笔记、读书导引
└── code/      ← 入门实验（组播最小工程等）
```

---

## notes · 章节笔记

| # | 主题 | 笔记 | HFT |
|---|------|------|-----|
| — | 实体书递进说明 | [note-DPDK实体书递进](./notes/note-DPDK实体书递进.md) | 🟡 |
| 1 | 认识 DPDK（实体书 Ch1） | [chapter-01-认识DPDK/](./chapter-01-认识DPDK/) · [stub](./notes/chapter-01-DPDK架构与EAL.md) | 🔴 |
| 2 | Cache 与内存（实体书 Ch2） | [chapter-02-Cache与内存/](./chapter-02-Cache与内存/) · [stub](./notes/chapter-02-Cache与内存.md) | 🔴 |
| 3 | mbuf、mempool | [chapter-02-mbuf](./notes/chapter-02-mbuf与内存池.md) | 🔴 |
| 4 | PMD、轮询模式 | [chapter-03](./notes/chapter-03-PMD与轮询模式.md) | 🔴 |
| 5 | 零拷贝、旁路原理 | [chapter-04](./notes/chapter-04-零拷贝与用户态旁路.md) | 🔴 |
| 6 | UDP 组播行情 | [chapter-05](./notes/chapter-05-组播行情接入.md) | 🔴 |
| 8 | 流分类与多队列（实体书 Ch8） | [chapter-08-流分类与多队列/](./chapter-08-流分类与多队列/) · [stub](./notes/chapter-08-流分类与多队列.md) | 🔴 |

---

## code · 实验

| 实验 | 路径 | 状态 |
|------|------|------|
| 组播行情最小工程 | [mcast-minimal/](./code/mcast-minimal/) | 待实现 |

---

## 读完之后

→ [02-Advanced-Book](../02-Advanced-Book/) · 《Linux 高性能网络详解》（RDMA / XDP / 选型）  
→ [10 总目录](../README.md)
