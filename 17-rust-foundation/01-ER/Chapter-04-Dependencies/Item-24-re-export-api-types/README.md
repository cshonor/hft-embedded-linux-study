# Item 24: Re-export dependencies whose types appear in your API

> **Effective Rust** · [Chapter 4 — Dependencies](../../ER-本书目录.md)  
> **中文**：重新导出 API 中出现其类型的依赖项  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [item-24-re-export](./demo/)

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `pub use` | [7.4 use 与 pub use](../../../00-Book/07-packages-modules/7.4-使用use关键字将名称引入作用域.md) |
| 对外 API 门面 | [14.2 发布 crate](../../../00-Book/14-cargo-crates/14.2-将crate发布到Crates.io.md) |
| 工作空间依赖解析 | [14.3 Cargo 工作空间](../../../00-Book/14-cargo-crates/14.3-Cargo工作空间.md) |
| Semver 与公开 API | [Item 21](../Item-21-semver/README.md) |
| 最小可见性 | [Item 22](../Item-22-minimize-visibility/README.md) |
| 依赖图 / duplicates | [Item 25](../Item-25-dependency-graph/README.md)（待补） |
| CI 监控 API 边界 | [Item 32](../../Chapter-05-Tooling/Item-32-ci/README.md)（待补） |

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
dep-lib API 用 rand 0.7 的类型
app 直接用 rand 0.8 构造对象
         ↓
同名 ThreadRng，不同版本 → 类型断层，无法传入 API
         ↓
用户被迫写 wrapper crate 对齐版本（极烦）
         ↓
库作者：pub use rand; → 下游用 dep_lib::rand 拿「对的那版」
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 24](../../ER-拓展索引.md#item-24)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 公开依赖 | API 里出现外部类型 = 与其 Semver 绑定 |
| 多版本 | 同名不同版 = **不同类型** |
| 必须做 | API 用了就要 **`pub use`** |
| 下游 | 用 `your_crate::dep` 构造匹配类型 |
| 报错 | trait not satisfied → 先查**版本重复** |
| 谨慎 | 能不用外部类型在 API 里就别用 |

