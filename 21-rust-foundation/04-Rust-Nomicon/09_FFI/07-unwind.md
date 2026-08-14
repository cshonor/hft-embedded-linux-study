# 7 · 异常与栈展开 (FFI and unwinding)

← [本章目录](./README.md) · 上一节：[06-nullable.md](./06-nullable.md) · 下一节：[08-opaque.md](./08-opaque.md)

---

**Rust `panic!` 跨入 C，或 C++ 异常跨入 Rust → UB**。

| 对策 | API |
|------|-----|
| 捕获 panic | **`catch_unwind`** 于 FFI 边界 |
| 显式允许展开 | `extern "C-unwind"`（特定场景） |

→ 源码：[src/unwind.rs](./src/unwind.rs)
