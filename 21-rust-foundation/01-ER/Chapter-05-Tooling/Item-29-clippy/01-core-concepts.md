# Item 29 · 核心知识点

← [Item 29 目录](./README.md)

### Clippy

- Rust 官方 lint 工具：`cargo clippy`（Cargo 插件）。
- 五类维度：

| 维度 | 关注点 |
|------|--------|
| **Correctness** | 常见逻辑错误 |
| **Idiom** | 不够 Rust 味 |
| **Concision** | 更简写法 |
| **Performance** | 多余分配 / 计算 |
| **Readability** | 降低理解成本 |

### 局部 / 全局豁免

```rust
#[allow(clippy::some_lint)]  // 单项
// crate 根 #![allow(...)]   // 整 crate（慎用）
```

---
