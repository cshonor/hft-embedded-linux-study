# 1 · 默认数据布局：`repr(Rust)`

← [本章目录](./README.md) · [00-overview.md](./00-overview.md) · 下一节：[02-exotic-types.md](./02-exotic-types.md)

---

## 对齐与填充 (Alignment and Padding)

所有类型都有按字节指定的**对齐要求**。为保证各字段正确对齐，Rust 会在字段之间自动插入 **padding** 字节。

## 字段重排 (Field Reordering)

与 C 不同，Rust **默认不保证** struct 字段在内存中的书写顺序（数组除外，始终密集且按序排列）。编译器会**重排字段**以消除对齐浪费，因此不同泛型实例可能有截然不同的布局。

→ 源码：[src/repr_rust.rs](./src/repr_rust.rs)（`repr(Rust)` vs `repr(C)` 的 `size_of` / `offset_of`）

## 空指针优化 (Null Pointer Optimization)

对某些枚举，若含单元变体（如 `None`）与**不可为空指针**变体（如 `Some(&T)`），Rust 可省去区分变体的 tag：直接将空指针解释为 `None`，使 `Option<&T>` 与 `&T` **同尺寸**。

→ 源码：[src/repr_rust.rs](./src/repr_rust.rs)（`Option<&T>` niche）
