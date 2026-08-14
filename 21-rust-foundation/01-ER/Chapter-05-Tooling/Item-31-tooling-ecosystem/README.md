# Item 31: Take advantage of the tooling ecosystem

> **Effective Rust** · [Chapter 5 — Tooling](../../ER-本书目录.md)  
> **中文**：充分利用工具生态系统  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [ ] demo（见 [ER-demos 索引](../../ER-demos/README.md) / Book demo）

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `rustup`、安装 | [1.1 安装](../../../00-Book/01-getting-started/1.1-安装.md) |
| `cargo` 基础 | [1.3 Hello Cargo](../../../00-Book/01-getting-started/1.3-Hello-Cargo.md) |
| 发布、doc | [14.2 Crates.io](../../../00-Book/14-cargo-crates/14.2-将crate发布到Crates.io.md) |
| CI 集成 | [Item 32](../Item-32-ci/README.md)（待补） |

---

## 一句话

**`rustfmt` on save**

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
cargo metadata (JSON)
         ↓
cargo_metadata crate → udeps / deny / …
         ↓
syn / quote / AST 级工具 → expand / tarpaulin / …
         ↓
日常痛点 → 搜 cargo-<name> → 有价值则 CI（Item 32）
         ↓
快且无 FP 的（rustfmt）→ 编辑器 save hook
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 31](../../ER-拓展索引.md#item-31)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 日常三件套 | **check / fmt / clippy** |
| 元数据 | **`cargo metadata`** 驱动生态工具 |
| 宏 | **`cargo-expand`** |
| 依赖 | **tree / udeps / deny** |
| 采纳 | 好用 → **CI**；fmt → **编辑器** |
| 选型 | 看维护度，别绑死废弃插件 |

