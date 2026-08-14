# Item 26: Be wary of feature creep

> **Effective Rust** · [Chapter 4 — Dependencies](../../ER-本书目录.md)  
> **中文**：警惕特性蔓延  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [item-26-feature-creep](./demo/)

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `Cargo.toml`、构建配置 | [1.3 Hello Cargo](../../../00-Book/01-getting-started/1.3-Hello-Cargo.md)、[14.1 发布配置](../../../00-Book/14-cargo-crates/14.1-采用发布配置自定义构建.md) |
| 条件编译 `#[cfg(test)]` | [11.3 测试的组织结构](../../../00-Book/11-testing/11.3-测试的组织结构.md) |
| Feature 统一 | [Item 25](../Item-25-dependency-graph/README.md) |
| `no_std` / `std` feature | [Item 33](../../Chapter-06-Beyond-Standard-Rust/Item-33-no-std/README.md)（待补） |
| CI 测 feature 组合 | [Item 32](../../Chapter-05-Tooling/Item-32-ci/README.md)（待补） |
| Clippy 否定性 feature 名 | [Item 29](../../Chapter-05-Tooling/Item-29-clippy/README.md)（待补） |

---

## 一句话

**少开 feature**

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
cfg（编译器底层）
         ↓
features（Cargo 包级开关）
         ↓
全局 unification → 特性必须「纯加法」
         ↓
互斥 feature / 字段门控 → 下游无法控、必炸
         ↓
克制数量 + CI 测组合（cargo-hack）
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 26](../../ER-拓展索引.md#item-26)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 原则 | Feature **只加能力**，互斥用 `cfg(target_*)` |
| 公开 API | **别** `#[cfg(feature)]` 门控 pub 字段/方法 |
| Unification | 全图**并集** — 用户控不了 |
| 命名 | 避 `no_*`；用 `std` 等正向名 |
| 数量 | 防 \(2^N\) — 少 feature + CI powerset |
| optional dep | 自动同名 feature |

