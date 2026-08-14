# Item 23 · 重点结论

← [Item 23 目录](./README.md)

### 默认：第三方 crate **不用** `*`

- 明确列出需要的类型 / trait / 函数。

### 合法例外

| 场景 | 示例 | 为何可接受 |
|------|------|------------|
| **测试** | `use super::*;` | 你控制模块与测试（Book 11.3） |
| **crate 内重导出** | `pub use thing::*;` | 同一 crate，你控制符号集（Item 22/24） |
| **库作者 curated prelude** | `use thing::prelude::*;` | 作者维护的小集合，生态惯例 |

### 硬要用 `*` 时的兜底

- `Cargo.toml` **精确锁定**版本（`=1.2.3`，见 Item 21）—— 仍不推荐，只是减缓 Minor 炸弹。

---
