# 2.1 Linting（Lint）

> 所属：**Additional Testing Tools** · [← 章索引](./README.md)

**Clippy** 纳入 CI — 除风格外，**correctness** 类 lint 抓「能编译但明显错」的模式。

## 实践

```bash
cargo clippy --all-targets -- -D warnings
```

- 仓库级 **`clippy.toml`** 统一阈值 → [ER-demos/clippy.toml](../../01-ER/ER-demos/clippy.toml)

ER → [Item 29 Clippy](../../01-ER/Chapter-05-Tooling/Item-29-clippy/README.md) · Book → [附录 D.4 实用工具](../../00-Book/appendix/D.4-实用开发工具.md)
