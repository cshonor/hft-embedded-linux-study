# M1 · Modern C++（现代 C++ 门槛）

> **里程碑定位：** 🔴 硬门槛 · 全书精读
> **学习顺序：** M0 之后（07 TLPI 之后、10 PNP 之前）
> **难度：** ⭐⭐⭐⭐

## 包含的书

| 目录 | 书 | 状态 |
|------|-----|------|
| [01-Effective-Modern-C++](./01-Effective-Modern-C++/) | Effective Modern C++ | 整章 README 已写，小节笔记拆分中 |

## 为什么是硬门槛

C++11/14 是分水岭，老 C++ 和现代 C++ 是两种写法。这 42 条不过，后面 muduo 的回调、移动语义、智能指针全部看天书。

## 验收

能读 muduo 里 `shared_ptr`/回调/移动语义不懵 → 再开 [10 PNP](../../04.5-network-sockets/)。

## 跨模块回读

读完 M1 后，回 [M0](../M0-entry-syntax/) 补 01 C++ Primer 的三章（M0 阶段跳过的进阶内容）：

| 章 | 目录 | 为什么 M1 之后读 |
|----|------|------------------|
| ch15 | [OOP](../M0-entry-syntax/01-C++Primer/ch15-oop/) | 继承/多态，先懂基类再谈继承 |
| ch16 | [模板与泛型](../M0-entry-syntax/01-C++Primer/ch16-templates/) | 模板需要现代 C++ 基础（`auto`/`decltype`/移动） |
| ch17 | [标准库特殊设施](../M0-entry-syntax/01-C++Primer/ch17-special-library-facilities/) | `tuple`/`regex`/`random`，用到再查 |

## 小节笔记结构

每章按 Item 拆成独立文件，每个文件包含：
- 这节讲什么
- 核心规则（代码 + 表格）
- 新手要点（和 C 的区别）
- HFT 关联
- 自测题
- 参考与延伸

模板示例：[Item 1 模板类型推导](./01-Effective-Modern-C++/ch01-deducing-types/item01-template-type-deduction.md)

## 章节清单（8 章 42 条款）

| 章 | 目录 | 条款数 |
|----|------|--------|
| 1 | [类型推导](./01-Effective-Modern-C++/ch01-deducing-types/) | 4 |
| 2 | [auto](./01-Effective-Modern-C++/ch02-auto/) | 2 |
| 3 | [移步现代 C++](./01-Effective-Modern-C++/ch03-moving-to-modern-cpp/) | 4 |
| 4 | [智能指针](./01-Effective-Modern-C++/ch04-smart-pointers/) | 5 |
| 5 | [右值/移动/转发](./01-Effective-Modern-C++/ch05-rvalue-move-forwarding/) | 8 |
| 6 | [Lambda](./01-Effective-Modern-C++/ch06-lambda-expressions/) | 4 |
| 7 | [并发 API](./01-Effective-Modern-C++/ch07-concurrency-api/) | 4 |
| 8 | [杂项](./01-Effective-Modern-C++/ch08-tweaks/) | 2 |

---

← [学习路线](../LEARNING-PATH.md) · [上一站 M0](../M0-entry-syntax/) · [下一站 M3 穿插](../M3-engineering-standards/)
