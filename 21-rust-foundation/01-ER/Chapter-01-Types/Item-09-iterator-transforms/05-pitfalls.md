# Item 9 · 易错细节

← [Item 9 目录](./README.md)

| 问题 | 说明 |
|------|------|
| **`for x in collection`** | 默认 **move** / `into_iter`，后面不能用 `collection` |
| 需保留集合 | `for x in &collection` 或 `.iter()` |
| **`collect()` 类型** | 编译器常推不出容器类型 → `collect::<Vec<_>>()` 或 `let v: Vec<_> = ...` |
| 闭包捕获 | `filter(\|x\| ...)` 中 `x` 是 `&&T`，比较时常要 `*x` |

---
