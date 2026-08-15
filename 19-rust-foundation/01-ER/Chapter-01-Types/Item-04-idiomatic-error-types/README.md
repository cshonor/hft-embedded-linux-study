# Item 4: Prefer idiomatic Error types

> **Effective Rust** · [Chapter 1 — Types](../../ER-本书目录.md)  
> **中文**：倾向使用符合惯例的 Error 类型  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [item-04-error-types](./demo/) · [item-04-core-error](./demo-core-error/)（no_std `Error`）

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `Result`、`?` | [9.2 Result](../../../00-Book/09-error-handling/9.2-Result-与可恢复的错误.md) |
| `Display` / `Debug` | [10.2 trait](../../../00-Book/10-generics-traits-lifetimes/10.2-trait.md) |
| Newtype | [Item 6](../Item-06-newtype-pattern/README.md)（ER） |
| Option/Result 链式 | [Item 3](../Item-03-option-result-transforms/README.md)（ER） |

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
String 当错误（不行：无法 impl Error）
  → Newtype 包装（MyError(String)）
  → Enum 聚合子错误 + 自定义 source()
  → 库：具体 Enum + thiserror
  → 应用：Box<dyn Error> + anyhow
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 04](../../ER-拓展索引.md#item-04)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 惯例 | 自定义 `E` 应 `Display + Debug + Error` |
| 孤儿 | 不能 `impl Error for String`；用 Newtype / enum |
| 库 | 具体 `enum` + `thiserror` |
| 应用 | `anyhow` / `Box<dyn Error>` |
| `?` | `From` 子错误，少 `map_err` |
| `source` | 链式暴露底层原因 |

