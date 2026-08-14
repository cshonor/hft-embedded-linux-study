# 1 · 隐式转换 (Coercions)

← [本章目录](./README.md) · [00-overview.md](./00-overview.md) · 下一节：[02-dot-operator.md](./02-dot-operator.md)

---

Rust 在特定上下文中**自动**转换类型，通常是对类型的「弱化」（改变指针类型、缩短生命周期等）。

- 目的：日常代码「开箱即用」，**基本无害**。
- **Trait 匹配时不自动转换**（方法调用的 **receiver** 除外）。

→ 源码：[src/coercions.rs](./src/coercions.rs)（`&String` → `&str`、`&T` → `&dyn Trait`、数组 unsizing）
