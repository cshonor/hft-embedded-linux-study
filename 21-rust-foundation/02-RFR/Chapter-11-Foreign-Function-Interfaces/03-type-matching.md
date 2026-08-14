# 2.1 Type Matching（类型匹配）

> 所属：**Types Across Language Boundaries** · [← 章索引](./README.md)

机器码层**无 Rust 类型信息** — 两侧须对**布局与 ABI** 一致理解。

## 整型

用 **`std::os::raw`** / **`libc`** 的 `c_int`、`c_char` 等 — 勿猜 C `int` 宽度。

## 结构体

**`#[repr(C)]`** — 与 C 交换的 struct 固定布局；默认 `repr(Rust)` **不保证**字段顺序。

## 枚举

**`#[repr(C, u8)]`** 等 — 对应 tagged union 心智模型（以参考手册为准）。

## Niche

**`Option<NonNull<T>>`**、**`Option<extern "C" fn(...)>`** — 利用 null 指针优化。

→ [第 2 章 Layout](../Chapter-02-Types/02-layout.md)
