# 1.3 Doctests（文档测试）

> 所属：**Rust Testing Mechanisms** · [← 章索引](./README.md)

`///` 中 fenced 代码块会**编译并运行**。

## 隐藏行

行首 **`# `** 的代码参与测试，但**不出现在**渲染文档 — 适合隐藏样板。

## `compile_fail`

断言代码**必须不能编译** — 验证 API 边界（如某类型非 `Send`）。

## `no_run` / `ignore`

- **`no_run`** — 只编译不运行（需外部资源时）。
- **`ignore`** — 跳过（慎用，易腐化）。

ER → [Item 27 文档 + doctest](../../01-ER/Chapter-05-Tooling/Item-27-document-public-api/README.md)
