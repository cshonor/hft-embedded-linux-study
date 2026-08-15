# Item 28 · 易错细节

← [Item 28 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **`$e:expr` 用多次** | 副作用 / 昂贵计算重复 |
| **可见性** | 宏只对定义**之后**可见；`#[macro_export]` → **crate 根** |
| **工具链** | 展开代码难 `rustfmt`；IDE 跳转差；一行 → 数百行 **code bloat** |
| **proc-macro crate** | 编译时任意 Rust 代码 → 依赖审计（Item 25） |

---
