# M5 · STL（用法 + 源码）

> **里程碑定位：** 🟡 按需查 · 不顺序读
> **学习顺序：** M1 之后穿插（M4 PNP 写容器/缓冲区时查；Effective STL 与 M1 EMC++ 配对读效果更佳）
> **难度：** ⭐⭐⭐（01）/ ⭐⭐⭐⭐⭐（02）

## 包含的书

| 目录 | 书 | 状态 |
|------|-----|------|
| [01-Effective-STL](./01-Effective-STL/) | Effective STL（50 条） | 整章 README 已写 |
| [02-STL-Source-Analysis](./02-STL-Source-Analysis/) | STL 源码剖析 | 整章 README 已写 |

## 怎么读

这一层不是"读"，是"查"。碰到具体问题再翻对应条目。

| 书 | 何时查 | 必记条目 |
|----|--------|----------|
| 01 Effective STL | 写容器/缓冲区时 | remove-erase 惯用法、reserve、迭代器失效 |
| 02 STL 源码剖析 | HFT 碰到性能瓶颈时 | allocator、vector 三指针、RB-tree、introsort |

## 注意

- 01 是 pre-C++11 时代条目，读时用 EMC++ 的现代写法校对（手写循环 → 算法 + lambda）
- 02 最重，时间紧可后补，等 HFT 主线进行中按需

## 小节笔记

当前为整章 README 粒度。如需按小节拆分（01 有 50 条），等读到时再拆。

---

← [学习路线](../LEARNING-PATH.md) · [上一站 M4 网络编程](../M4-cpp-network-programming/) · [下一站 M6 经典规范](../M6-Classic-Effective/)
