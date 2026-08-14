# Item 27: Document public interfaces

> **Effective Rust** · [Chapter 5 — Tooling](../../ER-本书目录.md)  
> **中文**：为公共接口编写文档  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [ ] demo（见 [ER-demos 索引](../../ER-demos/README.md) / Book demo）

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `///` / `//!`、doc test | [14.2 发布到 Crates.io](../../../00-Book/14-cargo-crates/14.2-将crate发布到Crates.io.md) |
| 类型表达语义 | [Item 1](../../Chapter-01-Types/Item-01-express-data-structures/README.md) |
| `# Panics` / 少 panic | [Item 18](../../Chapter-03-Concepts/Item-18-dont-panic/README.md) |
| CI 集成 doc | [Item 32](../Item-32-ci/README.md)（待补） |
| 工具生态 | [Item 31](../Item-31-tooling-ecosystem/README.md)（待补） |
| 最小可见性 | [Item 22](../../Chapter-04-Dependencies/Item-22-minimize-visibility/README.md) |

---

## 一句话

见 [03-key-takeaways.md](./03-key-takeaways.md)。

---

## 专项笔记（按需点开）

| # | 专题 | 阅读 |
|---|------|------|
| 01 | 核心知识点 | [01-core-concepts.md](./01-core-concepts.md) |
| 02 | 逻辑脉络 | [02-logic-flow.md](./02-logic-flow.md) |
| 03 | 重点结论 | [03-key-takeaways.md](./03-key-takeaways.md) |
| 04 | 案例与代码 | [04-examples.md](./04-examples.md) |
| 05 | 易错细节 | [05-pitfalls.md](./05-pitfalls.md) |


---

## 逻辑脉络

```text
Item 1：能进类型的语义 → 进类型（编译器强制）
         ↓
文档：只补类型表达不了的「为什么 / 契约 / 陷阱」
         ↓
doc test + CI cargo doc → 防示例与链接退化（Item 32）
         ↓
examples/ → 完整集成示例，非 doc test 片段
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 27](../../ER-拓展索引.md#item-27)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 分工 | **类型**表达能表达的；**文档**补 WHY |
| 禁止 | **复述签名** |
| 板块 | Examples / Panics / Errors / Safety |
| 质量 | doc test + `deny(broken_intra_doc_links)` |
| 长示例 | **`examples/`** + `cargo run --example` |
| 发布 | README → crates.io；API → docs.rs |

