# Item 24 · 核心知识点

← [Item 24 目录](./README.md)

### 公开依赖（Public dependency）

- 库 crate 的**公开 API**（函数签名、返回类型、公共 struct 字段等）直接出现**外部 crate 的类型** → 该依赖成为**公开依赖**。

### Cargo 多版本共存

- 同一二进制依赖图中，可**同时链接**同一 crate 的多个**不兼容**版本（如 `rand 0.7` + `rand 0.8`）。
- 同名不同类型 → Rust 视为**两种互不兼容的类型**。

### 重导出（Re-export）

```rust
pub use some_dependency;
// 或
pub use some_dependency::SomeType;
```

- 把依赖（或其类型）作为**本 crate 公开 API 的一部分**暴露给下游。

---
