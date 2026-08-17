# M3 · Engineering Standards（工程规范穿插）

> **里程碑定位：** 🟡 按需查 · 不顺序读
> **学习顺序：** M1 之后穿插（M5 PNP 写容器/缓冲区时查）
> **难度：** ⭐⭐⭐

## 包含的书

| 目录 | 书 | 状态 |
|------|-----|------|
| [01-Effective-C++](./01-Effective-C++/) | Effective C++（55 条） | 整章 README 已写 |
| [02-More-Effective-C++](./02-More-Effective-C++/) | More Effective C++（35 条） | 整章 README 已写 |
| [03-Effective-STL](./03-Effective-STL/) | Effective STL（50 条） | 整章 README 已写 |
| [04-STL-Source-Analysis](./04-STL-Source-Analysis/) | STL 源码剖析 | 整章 README 已写 |

## 怎么读

这一层不是"读"，是"查"。碰到具体问题再翻对应条目。

| 书 | 何时查 | 必记条目 |
|----|--------|----------|
| 01 Effective C++ | 写代码踩坑时 | const/inline/enum 替代 #define、多态基类析构加 virtual |
| 02 More Effective C++ | 同上 | 智能指针初版（已过时，看 04 代替） |
| 03 Effective STL | 写容器/缓冲区时 | remove-erase 惯用法、reserve、迭代器失效 |
| 04 STL 源码剖析 | HFT 碰到性能瓶颈时 | allocator、vector 三指针、RB-tree、introsort |

## 注意

- 02/03 是 pre-C++11 老规矩，部分条目被现代特性取代（如 `auto_ptr` 已废弃，用 `unique_ptr`）
- 04 最重，时间紧可后补，等 HFT 主线进行中按需

## 小节笔记

当前为整章 README 粒度。如需按小节拆分（02 有 55 条、05 有 50 条），等读到时再拆。

---

← [学习路线](../LEARNING-PATH.md) · [上一站 M1](../M1-modern-cpp/) · [下一站 M2 硬门槛](../M2-deep-principles/)
