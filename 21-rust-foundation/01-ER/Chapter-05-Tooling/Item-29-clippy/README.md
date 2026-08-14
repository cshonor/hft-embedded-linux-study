# Item 29: Listen to Clippy

> **Effective Rust** · [Chapter 5 — Tooling](../../ER-本书目录.md)  
> **中文**：倾听 Clippy 的建议  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [clippy.toml](../../ER-demos/clippy.toml)

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| Cargo 子命令 | [1.3 Hello Cargo](../../../00-Book/01-getting-started/1.3-Hello-Cargo.md) |
| `as` 与 cast lint | [Item 5](../../Chapter-01-Types/Item-05-type-conversions/README.md) |
| 通配符导入 | [Item 23](../../Chapter-04-Dependencies/Item-23-avoid-wildcard-imports/README.md) |
| feature 否定命名 | [Item 26](../../Chapter-04-Dependencies/Item-26-feature-creep/README.md) |
| `Copy` / `.clone()` | [Item 10](../../Chapter-02-Traits/Item-10-standard-traits/README.md) |
| CI `-Dwarnings` | [Item 32](../Item-32-ci/README.md)（待补） |
| 工具生态 | [Item 31](../Item-31-tooling-ecosystem/README.md)（待补） |

---

## 一句话

**修**

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
Effective Rust 许多条目 = 人工 Code Review 准则
         ↓
Clippy = 编译期自动化同一套审查
         ↓
零警告基线 + CI -Dwarnings → 新退化无处藏身
         ↓
每条 lint 带文档链接 → 初学者的惯用法导师
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 29](../../ER-拓展索引.md#item-29)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 命令 | **`cargo clippy -- -Dwarnings`** |
| 基线 | **零警告** 或显式 allow |
| 态度 | 顺应重构 > 争误报 |
| 角色 | 机械化 Effective Rust + 学习导师 |
| Pedantic | 可不启，值得读 |
| CI | 与 Item 32 绑定 |

