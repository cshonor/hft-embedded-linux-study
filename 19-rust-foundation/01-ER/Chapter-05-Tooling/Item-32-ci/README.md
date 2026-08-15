# Item 32: Set up a continuous integration (CI) system

> **Effective Rust** · [Chapter 5 — Tooling](../../ER-本书目录.md)  
> **中文**：建立持续集成 (CI) 系统  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [CI 示例](../../.github/workflows/er-study-ci.yml)

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `cargo test`、测试组织 | [11.1](../../../00-Book/11-testing/11.1-如何编写测试.md)、[11.3](../../../00-Book/11-testing/11.3-测试的组织结构.md) |
| `Cargo.lock`（app vs lib） | [Item 25](../../Chapter-04-Dependencies/Item-25-dependency-graph/README.md) |
| 工具清单 | [Item 31](../Item-31-tooling-ecosystem/README.md) |

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
ER 全书建议（Clippy、deny、features、doc…）
         ↓
仅文档 / 口头 → 很快腐化
         ↓
CI 自动化 → 真正防线
         ↓
流程 Bug（忘跑 codegen）→ 先加 CI 步骤再修（同 Item 30 TDD）
         ↓
全绿铁律 + 本地可复现 → 人不替机器背锅
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 32](../../ER-拓展索引.md#item-32)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 目的 | 自动化全书最佳实践 |
| 确定性 | **`rust-toolchain.toml`** |
| 节奏 | PR 快检 + 定期重检 + fuzz 后台 |
| 铁律 | **全绿**、无 flaky |
| 本地 | CI 命令 = 开发者能先跑 |
| 开源 | 限 fork CI、钉 Action SHA |
| bench | CI 结果**仅供参考** |

