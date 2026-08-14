# Item 15 · 易错细节

← [Item 15 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **format! / 临时 + 借引用** | 表达式结束临时 drop → 悬垂借用 |
| **多索引结构不同步** | `Vec` + `BTreeMap` 互存引用或脆弱 index，删除时易乱 → 智能指针或稳定 ID |
| **owner 仍想用数据** | 有 outstanding 借用时不能 move/drop；有 `&mut` 时 owner 不能读 |

---
