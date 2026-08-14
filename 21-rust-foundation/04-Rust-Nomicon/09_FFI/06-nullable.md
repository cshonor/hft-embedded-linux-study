# 6 · 可空指针优化 (Nullable pointer optimization)

← [本章目录](./README.md) · 上一节：[05-globals.md](./05-globals.md) · 下一节：[07-unwind.md](./07-unwind.md)

---

Rust 无 null；FFI 中 **`Option<extern "C" fn(...)>`** / **`Option<NonNull<T>>`** 的 `None` 即底层 `null`，无额外 tag。

→ 源码：[src/nullable.rs](./src/nullable.rs)
