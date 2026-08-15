# Item 30 · 易错细节

← [Item 30 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **examples 里 unwrap** | 示范坏习惯（Item 18） |
| **只信内置 bench** | 敏感、常需 nightly；用 **criterion** |
| **PR CI 跑 fuzz 数小时** | 成本爆炸；单独 job / 定时 |
| **纯二进制无 lib** | `tests/` 无法 `use main.rs` → 抽 **`lib.rs`**（Book 11.3） |
| **测类型已编码的契约** | 浪费；测集成与 I/O 边界 |

---
