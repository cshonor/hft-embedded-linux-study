# Item 15: Understand the borrow checker

> **Effective Rust** · [Chapter 3 — Concepts](../../ER-本书目录.md)  
> **中文**：理解借用检查器  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [item-15-borrow-checker](./demo/)

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| 借用规则 | [4.2 引用与借用](../../../00-Book/04-ownership/4.2-引用与借用.md) |
| 生命周期、NLL | [Item 14](../Item-14-lifetimes/README.md)、[10.3 专题](../../../00-Book/10-generics-traits-lifetimes/10.3-生命周期与引用有效性.md) |
| `RefCell` / 内部可变 | [15.5 RefCell](../../../00-Book/15-smart-pointers/15.5-RefCell与内部可变性.md) |
| `Arc` / `Mutex` | [16.3 共享状态](../../../00-Book/16-fearless-concurrency/16.3-共享状态并发.md) |
| 智能指针选型 | [Item 8](../../Chapter-01-Types/Item-08-references-pointers/README.md) |

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
C/C++ 裸指针：无借期、无读写互斥 → UAF / 数据竞争
Rust 引用 & / &mut + 借用检查器 → 编译期拦截

出借期间 owner 被压制：
  存在 &T   → owner 可读，不可改 / move / drop
  存在 &mut → owner 连读都不行，直到 &mut 结束
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 15](../../ER-拓展索引.md#item-15)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 规则 1 | 引用不能活过数据 |
| 规则 2 | 多个 `&` **或** 一个 `&mut` |
| Owner | 借出期间权限被压；只有 owner 能 move |
| 和解 | 延长 / 缩短 / 拆链 |
| 图/自引用 | `Rc<RefCell<_>>` 或索引；别硬裸引用 |
| replace | 从 `&mut Option` 里安全换值 |

