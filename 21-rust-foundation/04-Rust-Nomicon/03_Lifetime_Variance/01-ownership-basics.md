# 1 · 所有权与引用的基础模型

← [本章目录](./README.md) · [00-overview.md](./00-overview.md) · 下一节：[02-aliasing.md](./02-aliasing.md)

---

## 替代 GC

所有权系统解决 C/C++ 中悬垂指针、释放后使用等问题，**无需 GC** 即可安全管内存。

## 引用的两大黄金法则

1. 引用**绝不能**长于其 referent 的生命周期。
2. 可变引用 `&mut` **绝不能**被别名化（Aliased）。

→ 源码：[src/ownership.rs](./src/ownership.rs)（字段级拆分借用 — 编译器原生支持）
