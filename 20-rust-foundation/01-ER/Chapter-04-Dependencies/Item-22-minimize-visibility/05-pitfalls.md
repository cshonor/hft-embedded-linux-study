# Item 22 · 易错细节

← [Item 22 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **外围模块未 `pub`** | `somemodule` 私有 → 其内 `pub` 项外部仍不可达 |
| **`dead_code` 误读** | 「别的模块要用」却忘 `pub` / `pub(crate)` → 实际封闭在当前模块 |
| **`pub struct` ≠ 字段公开** | 常见误以为 struct `pub` 就全公开 |

---
