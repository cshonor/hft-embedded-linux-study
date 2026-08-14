# Item 10 · 易错细节

← [Item 10 目录](./README.md)

| 问题 | 说明 |
|------|------|
| **`Copy` 上 `.clone()`** | 多余；Clippy `clone_on_copy` |
| **derive `Ord` 顺序** | 按**字段声明顺序**字典比较，未必符合业务权重 → 手写 |
| **只 impl `PartialEq`** | 缺 `Eq` → 不能作需要 `Eq` 的 key |
| **含浮点求全序** | 不能 `Ord`；排序前过滤 NaN 或用手动规则 |

---
