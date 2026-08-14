# Item 27 · 案例与代码

← [Item 27 目录](./README.md)

### 死链检测

```rust
#![deny(broken_intra_doc_links)]

/// The bounding box for a [`Polygone`].  // 拼写错误
// → unresolved link to `Polygone`
```

### `examples/` 目录

- 完整可运行集成示例，**只调 public API**。
- `cargo run --example demo_name`
- 区别于 `tests/`（可测私有）和 doc test 短片段。

---
