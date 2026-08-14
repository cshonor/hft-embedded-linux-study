# Item 30: Write more than unit tests

> **Effective Rust** · [Chapter 5 — Tooling](../../ER-本书目录.md)  
> **中文**：编写除单元测试之外的更多测试  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [item-30-black-box](./demo/)

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `#[test]`、`#[cfg(test)]` | [11.1 如何编写测试](../../../00-Book/11-testing/11.1-如何编写测试.md) |
| 单元 / 集成 / `tests/` | [11.3 测试的组织结构](../../../00-Book/11-testing/11.3-测试的组织结构.md) |
| TDD 循环 | [12.4 TDD](../../../00-Book/12-cli-project/12.4-采用测试驱动开发完善库的功能.md) |
| doc test、examples | [Item 27](../Item-27-document-public-api/README.md)、[14.2](../../../00-Book/14-cargo-crates/14.2-将crate发布到Crates.io.md) |
| 类型 vs 测试分工 | [Item 1](../../Chapter-01-Types/Item-01-express-data-structures/README.md) |
| 测试里可 panic | [Item 18](../../Chapter-03-Concepts/Item-18-dont-panic/README.md) |
| bench / black_box | [Item 20](../../Chapter-03-Concepts/Item-20-avoid-over-optimize/README.md) |
| feature / cfg 穷举 | [Item 26](../../Chapter-04-Dependencies/Item-26-feature-creep/README.md) |
| CI matrix | [Item 32](../Item-32-ci/README.md)（待补） |

---

## 一句话

**`examples/` 里大量 `unwrap`**

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
单元测试 ── 内部细节
集成 / doc test ── 边界与契约
examples ── 用户视角整体用法
bench ── 性能（Item 20：先测再优化）
fuzz ── 不可信输入（解析器、网络协议…）
         ↓
类型系统已保证的（Item 1）→ 不必重复测
依赖行为漂移（Item 21）→ 测试作早期预警
         ↓
CI：matrix × features（Item 26/32）；fuzz 单独定期跑
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 30](../../ER-拓展索引.md#item-30)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 单元 | 同文件，可测 private |
| 集成 | `tests/`，仅 pub API |
| doc / examples | 文档测 + 用户向 demo |
| fuzz | 不可信输入 **必做** |
| bench | **`black_box`** / criterion |
| 修 bug | **先写 failing test** |
| 测试 vs 示例 | 测试可 unwrap；**examples 用 `?`** |

