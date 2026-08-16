# 03 · Ownership and Lifetimes · 本章定位

← [本章目录](./README.md) · [全书笔记](../notes.md) · 上一章：[02 Data Layout](../02_Data_Layout/README.md) · 下一章：[04 Type Cast](../04_Type_Cast/README.md)

---

官方标题 **Ownership and Lifetimes**。Rust 的灵魂，也是编写 Unsafe 时最需跨越的理论门槛。

| 对照 | 路径 |
|------|------|
| RFR 所有权 / 生命周期 | [04-ownership](../../02-RFR/Chapter-01-Foundations/04-ownership.md) · [08-lifetimes](../../02-RFR/Chapter-01-Foundations/08-lifetimes.md) |
| 可变引用与别名 | [06-mutable-references](../../02-RFR/Chapter-01-Foundations/06-mutable-references.md) |
| HRTB | [08-2-hrtb](../../02-RFR/Chapter-02-Types/08-2-hrtb.md) |
| split_at_mut | [19.1 unsafe demo](../../00-Book/19-advanced-features/19.1-unsafe-rust-demo/) |

**读完应能回答**：引用两大法则、型变三分类、何时需要 `PhantomData`、如何安全拆分借用。

---

## 小节路线图

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | 本页 |
| 1 | 所有权与引用 | [01-ownership-basics.md](./01-ownership-basics.md) |
| 2 | 别名与优化 | [02-aliasing.md](./02-aliasing.md) |
| 3 | 生命周期 | [03-lifetimes.md](./03-lifetimes.md) |
| 4 | 型变 | [04-variance.md](./04-variance.md) |
| 5 | Drop Check | [05-drop-check.md](./05-drop-check.md) |
| 6 | PhantomData | [06-phantom-data.md](./06-phantom-data.md) |
| 7 | 借用分离 | [07-split-borrows.md](./07-split-borrows.md) |
| — | 速记 · 自测 |

→ 从 [01-ownership-basics.md](./01-ownership-basics.md) 起读。
