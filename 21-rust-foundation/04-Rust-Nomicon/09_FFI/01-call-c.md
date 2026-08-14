# 1 · 从 Rust 调用 C (Calling foreign functions)

← [本章目录](./README.md) · [00-overview.md](./00-overview.md) · 下一节：[02-export-to-c.md](./02-export-to-c.md)

---

| 要点 | 说明 |
|------|------|
| 声明 | `extern "C" { fn foo(...); }` + `#[link(name = "...")]` |
| Unsafe | 所有 C 调用 **`unsafe`** — 编译器无法检查 C 的内存/线程安全 |
| 最佳实践 | 用 Rust 类型（`Vec`、切片）**包装**成 Safe API |

→ 源码：[src/call_c.rs](./src/call_c.rs)
