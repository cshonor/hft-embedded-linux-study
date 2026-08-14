# Item 31 · 逻辑脉络

← [Item 31 目录](./README.md)

```text
cargo metadata (JSON)
         ↓
cargo_metadata crate → udeps / deny / …
         ↓
syn / quote / AST 级工具 → expand / tarpaulin / …
         ↓
日常痛点 → 搜 cargo-<name> → 有价值则 CI（Item 32）
         ↓
快且无 FP 的（rustfmt）→ 编辑器 save hook
```

---
