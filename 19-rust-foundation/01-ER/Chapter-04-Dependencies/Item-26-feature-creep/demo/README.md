# Item 26: Feature 加法 + cargo-hack powerset

## 特性

| Feature | 作用 |
|---------|------|
| `std`（default） | 标准路径 |
| `schema` | 可选扩展（加法） |

## 穷举测试

```bash
cargo install cargo-hack
cd 01-ER/Chapter-04-Dependencies/Item-26-feature-creep/demo
cargo hack check --feature-powerset
cargo hack test --feature-powerset
```

CI 片段见 [Item 32](../../../Chapter-05-Tooling/Item-32-ci/README.md) 与 [er-study-ci.yml](../../../../.github/workflows/er-study-ci.yml)。

## docs.rs

```toml
[package.metadata.docs.rs]
all-features = true
```
