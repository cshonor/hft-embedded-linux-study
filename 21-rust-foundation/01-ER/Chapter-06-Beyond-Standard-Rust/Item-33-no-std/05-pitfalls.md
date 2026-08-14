# Item 33 · 易错细节

← [Item 33 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **`no_std` / `no_alloc` feature 名** | 违反加法；用 **`std` / `alloc`** 正向开启 |
| **文档路径 `src/core/`** | 该类型 **`no_std` 可用** |
| **隐式 `push`/`collect`** | OOM panic；受限环境改 fallible API |
| **只本地测 default features** | CI 必须 **no-default + bare-metal target** |

---
