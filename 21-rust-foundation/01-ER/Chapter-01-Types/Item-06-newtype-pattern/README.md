# Item 6: Embrace the newtype pattern

> **Effective Rust** · [Chapter 1 — Types](../../ER-本书目录.md)  
> **中文**：拥抱 newtype 模式  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [item-05-06-newtype](../Item-05-type-conversions/demo/)（derive_more）

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| 元组结构体 | [5.1 结构体](../../../00-Book/05-structs/5.1-定义并实例化结构体.md) |
| 孤儿规则、`Error` | [Item 4](../Item-04-idiomatic-error-types/README.md) |
| `From` / `Into` 单位转换 | [Item 5](../Item-05-type-conversions/README.md) |
| `Deref` 减少 `.0` | [15.2 Deref](../../../00-Book/15-smart-pointers/15.2-通过Deref将智能指针当作引用.md) · [15.2.1 勿滥用](../../../00-Book/15-smart-pointers/15.2.1-Deref嵌套可变与编译坑.md#反例-3过度-derefeffective-rust-item-6) |
| 布尔参数 → 枚举 | [Item 1](../Item-01-express-data-structures/README.md) |

---

## 一句话

**强业务语义、易混单位/ID**

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
裸 f64 / bool（语义易混）
  → type 别名（仅文档，无安全）
  → Newtype（编译期防混）
  → 必要时 From/TryFrom 做合法转换
  → 外部类型 + 外部 trait → Newtype 突破孤儿规则
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 06](../../ER-拓展索引.md#item-06)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| Newtype | 单字段 tuple struct = 新类型 |
| vs `type` | 别名不防混，Newtype 防混 |
| 孤儿 | 包成 local type 再 impl trait |
| `repr(transparent)` | 与内部同布局 |
| trait | 不继承，derive 或手写转发 |

