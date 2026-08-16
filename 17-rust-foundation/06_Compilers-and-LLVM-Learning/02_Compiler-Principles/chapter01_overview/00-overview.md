# 第 1 章 · 编译总览 · 本章定位

← [本章目录](./README.md) · 下一节：[01-basic-concepts-and-motivation.md](./01-basic-concepts-and-motivation.md)

---

第一章为全书**知识框架入口**，回答：

| 问题 | 在哪节 |
|------|--------|
| 编译器是什么？为什么要学？ | [§1](./01-basic-concepts-and-motivation.md) |
| 设计编译器不可违背的原则？ | [§2](./02-two-fundamental-principles.md) |
| 现代编译器如何分模块？ | [§3](./03-three-phase-structure.md) |
| 一次翻译具体做哪些事？ | [§4](./04-translation-pipeline-example.md) |
| 好编译器还应具备什么？ | [§5](./05-desired-properties.md) |

与 *Crafting Interpreters* [ch2 编译之山](../../01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/04-rust-hft-编译流水线对照.md) 互补：CI 偏**动手直觉**，本书偏**工程化三阶段 + 后端难点**。

---

## 全书一句话

> 编译器设计 = 在**正确性**、**实用性**、**算法**、**硬件限制**与**语言特性**之间做复杂 **Trade-offs** 的艺术与科学。
