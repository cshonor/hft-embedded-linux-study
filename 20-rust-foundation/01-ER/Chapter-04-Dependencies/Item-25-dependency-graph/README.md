# Item 25: Manage your dependency graph

> **Effective Rust** · [Chapter 4 — Dependencies](../../ER-本书目录.md)  
> **中文**：管理你的依赖图  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [WORKSPACE.md](../../ER-demos/WORKSPACE.md) · [Cargo.toml workspace.dependencies](../../../Cargo.toml)

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `Cargo.toml`、`Cargo.lock` | [1.3 Hello Cargo](../../../00-Book/01-getting-started/1.3-Hello-Cargo.md) |
| 工作空间、共享 lock | [14.3 Cargo 工作空间](../../../00-Book/14-cargo-crates/14.3-Cargo工作空间.md) |
| Semver、版本范围 | [Item 21](../Item-21-semver/README.md) |
| 公开依赖 / 多版本 | [Item 24](../Item-24-re-export-api-types/README.md) |
| 过程宏风险 | [Item 28](../../Chapter-05-Tooling/Item-28-macros-judiciously/README.md)（待补） |
| CI / Dependabot / deny | [Item 32](../../Chapter-05-Tooling/Item-32-ci/README.md)（待补） |
| FFI / ODR | [Item 34](../../Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/README.md)（待补） |

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
Cargo.toml  semver 范围 → 解析 → Cargo.lock 固化
         ↓
二进制：提交 lock → 可复现构建
库：     不提交 lock（下游忽略）→ 本地/CI 可保留 lock 测上游突变
         ↓
多版本：Rust 类型隔离 OK；API 暴露则灾难（Item 24）
FFI crate：ODR → 多版本 C 符号冲突，Cargo 魔法失效
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 25](../../ER-拓展索引.md#item-25)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 解析 | toml 范围 → lock 固化 |
| App lock | **要提交** |
| Lib lock | **别提交**给下游 |
| Features | 同版本取**并集** |
| 多版本 | Rust OK；**FFI 不行** |
| 工具 | tree / udeps / deny |
| 版本 | `"1"` 或 `"1.4.23"`，禁 `*` |

