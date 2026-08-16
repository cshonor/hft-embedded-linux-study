# Item 18: Don't panic

> **Effective Rust** · [Chapter 3 — Concepts](../../ER-本书目录.md)  
> **中文**：不要恐慌 / 避免使用 `panic!`  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [item-18-dont-panic](./demo/)

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `panic!` 机制 | [9.1 panic 与不可恢复的错误](../../../00-Book/09-error-handling/9.1-panic-与不可恢复的错误.md) |
| `Result` 与 `?` | [9.2 Result 与可恢复的错误](../../../00-Book/09-error-handling/9.2-Result-与可恢复的错误.md) |
| 何时 panic | [9.3 使用 panic 还是不用 panic](../../../00-Book/09-error-handling/9.3-使用panic还是不用panic.md) |
| 错误传播模式 | [Item 3](../../Chapter-01-Types/Item-03-option-result-transforms/README.md) |
| FFI / panic 边界 | [Item 16](../Item-16-avoid-unsafe/README.md)、[Item 34](../../Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/README.md) |
| 锁中毒 | [Item 17](../Item-17-shared-state-parallelism/README.md) |
| `no_panic` + CI | [Item 32](../../Chapter-05-Tooling/Item-32-ci/README.md)（待补） |

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
C++/Java 思维：catch_unwind ≈ try-catch
         ↓
阻碍 1：panic=abort → catch 根本跑不到
阻碍 2：状态撕裂 → exception safety 不成立
阻碍 3：panic 越过 FFI → UB
         ↓
库代码正路：Result + ? → 把决策交给调用者（passing the buck）
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 18](../../ER-拓展索引.md#item-18)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 默认 | **`Result` > `panic!`** |
| panic 用途 | 不可恢复 bug / 顶层 `main` |
| catch_unwind | 不是 try-catch；别用来「修」业务错误 |
| abort | 配置后 catch 无效 |
| 文档 | 公共 API 写 `# Panics` |
| 隐性 | unwrap、越界、除零 — 用机器查 |

