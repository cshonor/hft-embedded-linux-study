# Item 5: Understand type conversions

> **Effective Rust** · [Chapter 1 — Types](../../ER-本书目录.md)  
> **中文**：理解类型转换  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [item-05-06-newtype](../Item-05-type-conversions/demo/)（Deref 强制转换）

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| 数值、`as`、类型 | [3.2 数据类型](../../../00-Book/03-common-concepts/3.2-数据类型.md) |
| `Deref` 强制转换 | [15.2 Deref](../../../00-Book/15-smart-pointers/15.2-通过Deref将智能指针当作引用.md) · [15.2.1](../../../00-Book/15-smart-pointers/15.2.1-Deref嵌套可变与编译坑.md) |
| `From` / `Into` 与 `?` | [Item 4](../Item-04-idiomatic-error-types/README.md)、[9.2 Result](../../../00-Book/09-error-handling/9.2-Result-与可恢复的错误.md) |
| trait 对象强制转换 | [17.2 trait 对象](../../../00-Book/17-oop/17.2-为使用不同类型的值而设计的trait对象.md) |
| Newtype 转换 | [Item 6](../Item-06-newtype-pattern/README.md)（ER） |

---

## 一句话

**实现只写 `From`，边界只写 `Into`**

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

## 后续拓展

> 展开版：[ER-拓展索引 § Item 05](../../ER-拓展索引.md#item-05)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 数值 | 无隐式转换，必须显式 |
| 实现 | 只写 `From` |
| 边界 | 用 `Into` |
| 可能失败 | `TryFrom` + `Result` |
| `as` | 可有损；优先 trait 转换 |
| Coercion | Deref、切片、trait 对象等少数自动 |

