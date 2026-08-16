# Item 27 · 易错细节

← [Item 27 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **为关 warning 写废话** | `missing_docs` 逼出的垃圾注释 **比没有更糟** |
| **漏 `# Panics` / `# Safety`** | 违反 Item 18 或含 `unsafe` 却不写 → 最小惊吓原则破产 |
| **只写 crate 级 README** | crates.io 看 README；API 细节在 **docs.rs** + `//!` 模块文档 |
| **examples 测不到** | 长示例放 `examples/`，CI 应 `cargo test --examples` 或单独跑 |

---
