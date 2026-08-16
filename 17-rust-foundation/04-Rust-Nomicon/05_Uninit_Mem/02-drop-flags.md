# 2 · 丢弃标志 (Drop Flags)

← [本章目录](./README.md) · 上一节：[01-checked.md](./01-checked.md) · 下一节：[03-unchecked.md](./03-unchecked.md)

---

条件初始化 / 去初始化 / 重新赋值时，离开作用域或覆盖变量是否应 **Drop** 旧值？Rust 在栈上用 **drop flags** 跟踪初始化状态。

| 情况 | 行为 |
|------|------|
| 路径复杂、编译期无法确定 | **运行时**检查 flag 再决定是否 Drop |
| 可静态推导 | 优化为**无运行时开销** |

→ 源码：[src/drop_flags.rs](./src/drop_flags.rs)（条件构造带 `Drop` 的类型）
