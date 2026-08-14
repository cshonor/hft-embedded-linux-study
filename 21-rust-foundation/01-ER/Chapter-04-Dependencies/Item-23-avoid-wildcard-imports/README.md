# Item 23: Avoid wildcard imports

> **Effective Rust** · [Chapter 4 — Dependencies](../../ER-本书目录.md)  
> **中文**：避免通配符导入  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [ ] demo（见 [ER-demos 索引](../../ER-demos/README.md) / Book demo）

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `use`、glob `*` | [7.4 使用 use 引入名称](../../../00-Book/07-packages-modules/7.4-使用use关键字将名称引入作用域.md) |
| 测试里 `use super::*` | [11.3 测试的组织结构](../../../00-Book/11-testing/11.3-测试的组织结构.md) |
| Minor 加 API 仍「兼容」 | [Item 21](../Item-21-semver/README.md) |
| 可见性 / 重导出 | [Item 22](../Item-22-minimize-visibility/README.md)、[Item 24](../Item-24-re-export-api-types/README.md)（待补） |
| Clippy 警告 | [Item 29](../../Chapter-05-Tooling/Item-29-clippy/README.md)（待补） |

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
Item 21：Minor 加公开 API = Semver 兼容
         ↓
use external::* + cargo update → 新符号静默出现
         ↓
普通类型同名：本地定义优先 → 往往无害
Trait 方法同名：方法解析歧义 → 编译失败（E0034）
         ↓
对策：第三方 crate 显式 use；例外见 §3
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 23](../../ER-拓展索引.md#item-23)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 规则 | **第三方不用 `*`** |
| 例外 | 测试、`pub use`、curated prelude |
| Semver | Minor 加符号 + glob = 静默风险 |
| 类型冲突 | 本地优先，常无害 |
| trait 冲突 | 方法歧义 → **E0034** |
| 兜底 | `=x.y.z` 锁版本（下策） |

