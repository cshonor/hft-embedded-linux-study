# Item 25 · 易错细节

← [Item 25 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **`-` vs `_`** | crate 名 `some-crate` → 代码里 `some_crate` |
| **FFI 多版本** | 纯 Rust 可多版本；含 C/C++ → **ODR 链接冲突**（Item 34） |
| **`default-features = false` 无效** | 图中任一依赖未关默认 → **unification 并集**仍开默认 feature |
| **1.x 与 1.y 不会共存** | 只有 **incompatible** 版本才双份进图 |

---
