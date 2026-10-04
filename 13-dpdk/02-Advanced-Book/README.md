# 02-Advanced-Book · 《Linux 高性能网络详解》

> **13-DPDK** 二级目录 · **梯度 ②** — DPDK 深度、RDMA、XDP 与方案选型  
> **前置：** [01-Intro-Book](../01-Intro-Book/) + `02` CSAPP + `16` SysPerf + perf 已定位网络瓶颈。

---

## 目录结构

```
02-Advanced-Book/
├── notes/     ← RDMA、XDP、OpenOnload 等进阶笔记（含代码骨架与选型决策树）
└── code/      ← 同口径对照实验框架（内核栈/AF_XDP/DPDK/Onload 实测回填）
```

---

## notes · 进阶笔记

| 主题 | 笔记 | HFT |
|------|------|-----|
| OpenOnload / RDMA / RoCE 与 DPDK 取舍（verbs 语义、ibverbs 骨架、RoCE 无损坑、选型决策树） | [note-openonload-rdma对比](./notes/note-openonload-rdma对比.md) | 🟡 |
| XDP / tc-BPF / AF_XDP 与 DPDK 对照（XDP 程序骨架、AF_XDP ring 代码、渐进路线） | [note-XDP与DPDK对照](./notes/note-XDP与DPDK对照.md) | 🟡 |

→ 全书 BPF/XDP 深入：[06.7-BPF note-XDP](../../06.7-bpf-observability/02-bpf-performance-tools/note-XDP与tc-BPF.md)

---

## code · 实验

| 实验 | 路径 | 状态 |
|------|------|------|
| 同口径对照实验框架（E0–E4 实验矩阵 + 结果回填表） | [code/](./code/README.md) | 框架已建，E1–E4 待实测 |

> 进阶实验复用 [01-Intro-Book/code/mcast-minimal/](../01-Intro-Book/code/mcast-minimal/)
> 的同口径测法：DPDK 版 vs 内核栈版对照，换掉变量（XDP / Onload）再测一轮即可。

---

## 相关

- 入门梯度 → [01-Intro-Book](../01-Intro-Book/)
- [10 总目录](../README.md) · [OUTLINE](../OUTLINE.md)
