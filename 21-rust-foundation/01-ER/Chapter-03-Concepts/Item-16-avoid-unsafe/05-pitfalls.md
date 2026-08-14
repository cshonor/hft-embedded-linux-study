# Item 16 · 易错细节

← [Item 16 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **并发 + unsafe** | 借用检查不保护裸指针；data race 责任全在你 |
| **FFI 想当然** | C 指针生命周期、对齐、ABI 与 Rust 引用**不能默认一致** |
| **大块 unsafe** | 安全逻辑混进 unsafe 块 → 难以审计、难以 Miri |

---
