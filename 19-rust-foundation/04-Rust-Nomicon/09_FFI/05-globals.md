# 5 · 外部全局变量 (Foreign globals)

← [本章目录](./README.md) · 上一节：[04-interop.md](./04-interop.md) · 下一节：[06-nullable.md](./06-nullable.md)

---

C 库常导出 `static` 全局状态 → Rust 用 `extern { static mut ... }` 声明；**读写均为 unsafe**，极度危险。

→ 注释见 [src/globals.rs](./src/globals.rs)
