# 04 · Type Conversions · 本章定位

← [本章目录](./README.md) · [全书笔记](../notes.md) · 上一章：[03 Lifetime](../03_Lifetime_Variance/README.md) · 下一章：[05 Uninit Mem](../05_Uninit_Mem/README.md)

---

官方标题 **Type Conversions**。底层编程中，二进制位表示与类型处理至关重要。本章按**从安全自动到极度危险**的顺序讲解转换机制。

| 对照 | 路径 |
|------|------|
| ER 类型转换 | [Item 05](../../01-ER/Chapter-01-Types/Item-05-type-conversions/README.md) |
| Deref / 点运算 | [Book 15.2 Deref](../../00-Book/15-smart-pointers/15.2-通过Deref将智能指针当作引用.md) |
| RFR casting | [08-casting](../../02-RFR/Chapter-09-Unsafe-Code/08-casting.md) |
| 数据布局前提 | [02 Data Layout](../02_Data_Layout/00-overview.md) |

**读完应能回答**：隐式转换何时发生、点运算符的查找顺序、`as` 与 transmute 的危险边界。

---

## 小节路线图

```text
01  隐式转换 (Coercions) → coercions.rs
  ↓
02  点运算符查找链 → dot_operator.rs
  ↓
03  显式 as cast（非传递性）→ casts.rs
  ↓
04  transmute 终极警告 → transmute.rs
  ↓
05 Uninit Mem
```

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | 本页 |
| 1 | 隐式转换 | [01-coercions.md](./01-coercions.md) |
| 2 | 点运算符 | [02-dot-operator.md](./02-dot-operator.md) |
| 3 | 显式转换 `as` | [03-casts.md](./03-casts.md) |
| 4 | transmute | [04-transmutes.md](./04-transmutes.md) |
| — | 速记 · 自测 |

---

## 一句话

**转换阶梯章** — 无害 coercion → 点运算符 Deref/unsizing → `as` 显式边界 → transmute 禁区。

→ 从 [01-coercions.md](./01-coercions.md) 起读；源码从各节链到 `src/`。
