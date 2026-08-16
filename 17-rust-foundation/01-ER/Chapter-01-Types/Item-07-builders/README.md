# Item 7: Use builders for complex types

> **Effective Rust** · [Chapter 1 — Types](../../ER-本书目录.md)  
> **中文**：为复杂类型使用建造者模式  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [ ] demo（见 [ER-demos 索引](../../ER-demos/README.md) / Book demo）

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| 结构体、字段初始化 | [5.1 结构体](../../../00-Book/05-structs/5.1-定义并实例化结构体.md) |
| `Default`、`..` 更新语法 | [3.1 变量和可变性](../../../00-Book/03-common-concepts/3.1-变量和可变性.md) §Default |
| 方法、`self` / `&mut self` | [5.3 方法语法](../../../00-Book/05-structs/5.3-方法语法.md)、[Item 2](../Item-02-express-common-behavior/README.md) |
| 所有权、move | [4.1 所有权](../../../00-Book/04-ownership/4.1-什么是所有权.md) |

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
全字段手写初始化（冗长、易随字段变化腐化）
  → Default + ..（字段都要 Default）
  → Builder（构造逻辑与类型解耦，可选字段友好）
  → derive_builder 等宏（少手写样板）
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 07](../../ER-拓展索引.md#item-07)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 为何 Builder | 多可选字段、无全局 Default |
| 消费型 | 好链式；if 要重绑；build 一次 |
| 借用型 | 好分支；先 `let mut builder` 再改 |
| 宏 | 优先 `derive_builder` |
| 校验 | 放在 `build()` |

