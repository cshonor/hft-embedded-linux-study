# 2 · 从 C 调用 Rust (Calling Rust from C)

← [本章目录](./README.md) · 上一节：[01-call-c.md](./01-call-c.md) · 下一节：[03-callbacks.md](./03-callbacks.md)

---

| 要点 | 说明 |
|------|------|
| 调用约定 | `pub extern "C" fn` |
| 符号名 | `#[no_mangle]`（新版亦见 `#[unsafe(no_mangle)]`）禁用名称重整 |
| 产物 | `Cargo.toml` 中 `crate-type = ["cdylib"]` 生成 C 动态库 |

→ 源码：[src/export_to_c.rs](./src/export_to_c.rs)
