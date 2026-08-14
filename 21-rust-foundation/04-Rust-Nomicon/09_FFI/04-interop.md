# 4 · 互操作与数据表示 (Interoperability)

← [本章目录](./README.md) · 上一节：[03-callbacks.md](./03-callbacks.md) · 下一节：[05-globals.md](./05-globals.md)

---

| 类型 | 规则 |
|------|------|
| 结构体 | 跨边界须 **`#[repr(C)]`** |
| 字符串 | Rust `str` 无 `\0` → 用 **`CString`** |
| 可变参数 | `extern` 块可**声明** C 的 `...`；Rust **不能定义** variadic fn |

→ 源码：[src/interop.rs](./src/interop.rs)
