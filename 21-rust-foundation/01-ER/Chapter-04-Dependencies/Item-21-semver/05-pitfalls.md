# Item 21 · 易错细节

← [Item 21 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **「兼容」加 API 仍可能伤用户** | 新函数/类型/带默认的 trait 方法通常算 MINOR；但若用户 `use crate::*`，可能**命名冲突**（Item 23） |
| **库 crate 提交 lock 无助于下游** | 下游构建**忽略**库的 `Cargo.lock`；lock 只为**本仓库 CI** —— 须配 Dependabot，否则间接依赖冻结 |
| **Hyrum 定律** | 内部「无害」改动也可能 break 依赖未文档化行为的用户 |

---
