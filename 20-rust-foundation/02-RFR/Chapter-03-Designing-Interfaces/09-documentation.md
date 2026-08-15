# 3.1 Documentation（文档）

> 所属：**Obvious** · [← 章索引](./README.md)

除「做什么」外，文档应写清**失败方式、不变式与 unsafe 契约**。

## 必写项

| 主题 | 内容 |
|------|------|
| **Panic** | 哪些前置条件破坏会 panic、`unwrap` 何时炸 |
| **错误** | `Result` 各变体含义、是否可重试、是否 transient |
| **`unsafe`** | safety invariants：调用方必须保证什么 |
| **线程** | 是否 `Send`/`Sync`、是否需在特定线程调用 |

## 示例与 doctest

- `# Examples` 展示**推荐用法**（含错误处理）。
- `# Panics` / `# Errors` / `# Safety` 章节与 std 风格对齐。

ER → [Item 27 文档公开 API](../../01-ER/Chapter-05-Tooling/Item-27-document-public-api/README.md) · Book → [14.1 发布 crate](../../00-Book/14-cargo-crates/14.1-发布到-crates-io.md)
