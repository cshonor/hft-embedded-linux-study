# 7 · 借用分离（Splitting Borrows）

← [本章目录](./README.md) · 上一节：[06-phantom-data.md](./06-phantom-data.md)

---

借用检查器**原生支持**同一 struct **不同字段**的可变借用。但对数组、切片、树等，检查器无法理解内部**不相交性**。

须用 unsafe（raw ptr）在证明不重叠后拆分可变借用，如 `split_at_mut`、自定义可变迭代器。

→ 源码：[src/split_borrows.rs](./src/split_borrows.rs)

→ 下一章：[04 Type Cast](../04_Type_Cast/README.md)
