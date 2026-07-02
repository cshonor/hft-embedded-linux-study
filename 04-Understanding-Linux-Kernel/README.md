# 16 · Understanding the Linux Kernel 3rd

**ULK** · Daniel P. Bovet & Marco Cesati · [返回 LEARNING-CHAIN](../LEARNING-CHAIN.md)

> **定位：** **LKD 与源码之间的桥梁** — LKD 讲子系统 **做什么**；ULK 拆 **数据结构、算法、关键路径源码**（基于 **Linux 2.6** 时代，概念仍适用，版本号需对照 modern kernel）。  
> **深度 VM：** 仍由 [05-Linux-Virtual-Memory-Manager](../05-Linux-Virtual-Memory-Manager/) 负责；ULK Ch 8–9/17 作 **内核 MM 总览** 与 Gorman 衔接。

---

## 在 Linux 链上的位置

```
05 LKD        … 子系统功能、API、设计（Love · 3rd · 偏 modern）
06 ULK        … 实现细节、数据结构、2.6 源码走读（Bovet · 3rd）  ← 本模块
07 Gorman     … 虚拟内存专著
08 TLPI       … 用户态 syscall 接口
12 Rosen      … 内核网络栈
```

**推荐阅读序：** `05 LKD` 通读或并行 → **`06 ULK` 选章精读**（调度/中断/内存/syscall/IPC）→ `07 Gorman` 内存深潜。

---

## 文档

| 文件 | 说明 |
|------|------|
| [OUTLINE.md](./OUTLINE.md) | 20 章 + 附录 · HFT 标签 |
| [LEARNING_PLAN.md](./LEARNING_PLAN.md) | 与 LKD/Gorman 章节对照 |

---

## 为何不读 Linux 0.11 专著

现代 **5.x/6.x** 调度、CFS、内存管理、网络栈与 0.11 **架构差异过大**；0.11 仅作闲暇拓展，**不纳入**本仓库主线（与你的规划一致）。

---

## 进度

- [ ] Ch 1–7 基础（进程/中断/调度）🔴
- [ ] Ch 8–9 内存与地址空间 🔴
- [ ] Ch 10–11 syscall / signal 🟡
- [ ] Ch 12–18 VFS/块/Page Cache ⚪（HFT 热路径跳过）
- [ ] Ch 19–20 IPC / exec 🟡
