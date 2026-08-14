# 2 · 尺寸特殊的类型 (Exotically Sized Types)

← [本章目录](./README.md) · 上一节：[01-repr-rust.md](./01-repr-rust.md) · 下一节：[03-alt-repr.md](./03-alt-repr.md)

---

## 动态大小类型 (DSTs)

大小与对齐在**编译期未知**的类型：特征对象（`dyn Trait`）、切片（`[T]`、`str`）。不能直接按值存储，只能位于指针之后；指针须携带额外信息（切片长度、vtable 等），称为**胖指针 (wide pointer)**。

→ 源码：[src/exotic.rs](./src/exotic.rs)

## 零大小类型 (ZSTs)

完全不占内存的类型（如 `struct Nothing;`）。load/store 在底层为 **no-op**。Safe Rust 可忽略；**Unsafe** 中须注意：对 ZST 的指针偏移也是 no-op，且分配器通常要求**非零**分配大小。

## 空类型 (Empty Types)

无法实例化的类型，如 `enum Void {}`。用于类型系统表达不可达状态，例如 `Result<T, Void>` 在类型层面保证**绝不会**返回 `Err`。

→ 源码：[src/exotic.rs](./src/exotic.rs)
