# Item 25 · 案例与代码

← [Item 25 目录](./README.md)

### `cargo tree` 常用 flags

```bash
# 谁依赖了 problematic-crate？
cargo tree --invert problematic-crate

# 哪些 feature 被激活？
cargo tree --edges features

# 同包多版本？
cargo tree --duplicates
```

配合 Item 24：见 `rand` 重复 → 查公开 API 是否需 `pub use`。

### `Cargo.lock` 策略

| 项目类型 | lock 文件 |
|----------|-----------|
| **Binary / app** | **提交** VCS |
| **Library** | **不提交**（下游不用）；本地/CI 可保留自测 |

---
