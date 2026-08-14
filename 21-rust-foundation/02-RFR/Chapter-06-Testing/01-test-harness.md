# 1.1 The Test Harness（测试脚手架）

> 所属：**Rust Testing Mechanisms** · [← 章索引](./README.md)

`cargo test` 驱动 `rustc` 以 **`--test`** 等方式生成**测试二进制**：内置 `main` 发现并运行 `#[test]`。

## 自定义脚手架

```toml
[[test]]
name = "integration"
harness = false
path = "tests/custom.rs"
```

- **`harness = false`** — 自行实现 `main`，控制顺序、集成测试、模糊测试入口等。

Book → [11.1 如何编写测试](../../00-Book/11-testing/11.1-如何编写测试.md)
