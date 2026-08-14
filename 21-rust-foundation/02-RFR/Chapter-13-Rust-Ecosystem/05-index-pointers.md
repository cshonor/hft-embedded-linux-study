# 2.1 Index Pointers（索引指针）

> 所属：**Patterns in the Wild** · [← 章索引](./README.md)

节点放进 **`Vec<T>`**，用 **`NodeId`（`usize`/`u32`）** 代替大量 **`Rc`/引用** 织网。

| 优点 | 代价 |
|------|------|
| 生命周期可控、缓存局部性好 | 索引失效；需 **Generational Index** 等约定 |

生态参考：**`petgraph`**、有序 **`indexmap`**。

ER → [Item 15 借用检查器 · 图](../../01-ER/Chapter-03-Concepts/Item-15-borrow-checker/README.md)
