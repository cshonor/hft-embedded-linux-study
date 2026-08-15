# Item 10: Familiarize yourself with standard traits

> **Effective Rust** · [Chapter 2 — Traits](../../ER-本书目录.md)  
> **中文**：熟悉标准 trait  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [ ] demo（见 [ER-demos 索引](../../ER-demos/README.md) / Book demo）

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| trait 基础 | [10.2 trait](../../../00-Book/10-generics-traits-lifetimes/10.2-trait.md) |
| `Copy` / 移动 | [4.1 所有权](../../../00-Book/04-ownership/4.1-什么是所有权.md) |
| `Default` | [3.1 变量和可变性](../../../00-Book/03-common-concepts/3.1-变量和可变性.md) |
| `HashMap` 需要 `Hash + Eq` | [8.3 HashMap](../../../00-Book/08-collections/8.3-hashmap.md) |
| 操作符 trait | [19.2 高级 trait](../../../00-Book/19-advanced-features/19.2-高级trait.md) |
| 常用 trait 一览 | [Item 2](../Item-02-express-common-behavior/README.md) §标准 trait |

---

## 一句话

**`Copy` 要克制**

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
Clone ← Copy（Copy 必须 Clone）
PartialEq ← Eq（Eq 是标记）
PartialEq + PartialOrd ← Ord
Eq + Hash → 必须 hash(x)==hash(y) when x==y
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 10](../../ER-拓展索引.md#item-10)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| Trait | 一句 |
|-------|------|
| `Copy` | 小纯数据；改 Move→按位拷 |
| `Debug` / `Display` | `:?` 可 derive；`{}` 手写 |
| `Eq` | 标记 + 反射；浮点不行 |
| `Hash` + `Eq` | 相等则 hash 相等 |
| `Ord` derive | 字段顺序 ≠ 业务优先级 |

