# Item 3 · 易错细节

← [Item 3 目录](./README.md)

| 问题 | 说明 |
|------|------|
| **`#[must_use]`** | 忽略 `Result` 返回值会**警告**；故意忽略用 `let _ = ...` 明示 |
| **滥用 `unwrap`** | 等同「这里失败就 panic」；公共库、可恢复场景应避免 |
| **忘记 `as_ref`** | 在 `&Option<T>` 上直接 `map` / `unwrap_or` 可能触发 move 错误 |

---
