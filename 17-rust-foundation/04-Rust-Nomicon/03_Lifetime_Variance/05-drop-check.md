# 5 · 丢弃检查（Drop Check）

← [本章目录](./README.md) · 上一节：[04-variance.md](./04-variance.md) · 下一节：[06-phantom-data.md](./06-phantom-data.md)

---

确保调用泛型类型的 **Drop** 时，不会访问已销毁的借用数据。

- **基本规则**：泛型参数的生命周期须**严格长于**包含它的类型。
- **`#[may_dangle]`**（unstable）：标准库「逃生舱」，向编译器保证 Drop **绝不**碰可能过期的数据。

→ 笔记级概念；`Vec` 等标准库类型依赖此机制。
