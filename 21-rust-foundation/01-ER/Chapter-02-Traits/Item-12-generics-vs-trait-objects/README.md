# Item 12: Understand generics vs trait objects trade-offs

> **Effective Rust** · [Chapter 2 — Traits](../../ER-本书目录.md)  
> **中文**：理解泛型与 trait 对象之间的权衡  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [ ] demo（见 [ER-demos 索引](../../ER-demos/README.md) / Book demo）

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| 泛型、trait bound | [10.1 泛型](../../../00-Book/10-generics-traits-lifetimes/10.1-泛型数据类型.md)、[10.2 trait](../../../00-Book/10-generics-traits-lifetimes/10.2-trait.md) |
| `dyn Trait`、trait 对象 | [17.2 trait 对象](../../../00-Book/17-oop/17.2-为使用不同类型的值而设计的trait对象.md) |
| 胖指针 | [Item 8](../../Chapter-01-Types/Item-08-references-pointers/README.md) |
| 单态化、高级 trait | [19.2 高级 trait](../../../00-Book/19-advanced-features/19.2-高级trait.md) |
| 闭包 vs `fn` 静/动分发 | [Item 2](../../Chapter-01-Types/Item-02-express-common-behavior/README.md) |

---

## 一句话

**`stamp.make_copy()`**

---

## 专项笔记（按需点开）

| # | 专题 | 阅读 |
|---|------|------|
| 01 | 核心知识点 | [01-core-concepts.md](./01-core-concepts.md) |
| 02 | 逻辑脉络 | [02-logic-flow.md](./02-logic-flow.md) |
| 03 | 重点结论 | [03-key-takeaways.md](./03-key-takeaways.md) |
| 04 | 案例与代码 | [04-examples.md](./04-examples.md) |
| 05 | 易错细节 | [05-pitfalls.md](./05-pitfalls.md) |
| 06 | 单态化与静/动态分发（大白话） | [06-dispatch-beginner-guide.md](./06-dispatch-beginner-guide.md) |
| 07 | `dyn Trait` 是 DST，不能裸用 | [07-dyn-trait-dst-carriers.md](./07-dyn-trait-dst-carriers.md) |


---

## 逻辑脉络

```text
泛型：编译期特化 → 快、可组合多 bound → 代码体积↑
dyn：  运行期 vtable → 异构集合、省代码 → 间接调用开销
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 12](../../ER-拓展索引.md#item-12)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 单态化 | 编译期一型一份代码；同型跨文件只一份 |
| 并存 | impl 有本体代码；用 dyn 才加 vtable；两路不冲突 |
| 静态 | 编译硬编码地址，运行时零查表 |
| 动态 | vtable 编译生成、运行查表；终点仍是同一份方法代码 |
| `dyn` DST | 不能裸用；必须 `&` / `&mut` / `Box` 等包裹 → §07 |
| 泛型 | 静态分发；快；可能膨胀 |
| `dyn` | vtable 动态分发；异构集合；间接开销 |
| 对象安全 | 无泛型方法；无裸 `Self` |
| `Self: Sized` | 保留 dyn，方法仅具体类型可调 |
| 默认 | 先泛型，真要擦除再用 dyn |
| 大白话 | → [06-dispatch-beginner-guide.md](./06-dispatch-beginner-guide.md) |

