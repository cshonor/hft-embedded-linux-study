# M2 · C++ Object Model（对象模型）

> **里程碑定位：** 🔴 硬门槛 · 开 HFT 前必过
> **学习顺序：** M1 之后、M3 并发之前（03 TLPI / 03.5 UNP 之后）
> **难度：** ⭐⭐⭐⭐⭐

## 包含的书

| 目录 | 书 | 状态 |
|------|-----|------|
| [01-Cpp-Object-Model](./01-Cpp-Object-Model/) | 深度探索 C++ 对象模型 | 整章 README 已写，小节笔记待拆 |

## 为什么是硬门槛

HFT 里每个字节的布局都算钱。不懂 vptr/vtable、继承布局、placement new，HFT 代码看不懂也写不出。

## 阅读建议

- **精读 Ch1–Ch6**，Ch7 模板实例化可略读。
- 先懂对象怎么布局（静态），再进 M3 理解线程怎么交互（动态）。

## 验收

能解释空类 sizeof、多继承 this 调整、虚函数代价、CRTP 静态多态、placement new 场景。

## 小节笔记结构

每章按小节拆成独立文件，结构同 M1。7 章，约 27 个小节。

---

← [学习路线](../LEARNING-PATH.md) · [上一站 M1](../M1-modern-cpp/) · [下一站 M3 并发](../M3-concurrency/)
