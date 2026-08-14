# Item 24 · 易错细节

← [Item 24 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **误导性 trait 报错** | `ThreadRng: RngCore is not satisfied` — 常是 **0.8 的 RngCore vs 0.7 的 RngCore**，不是「忘 import trait」 |
| **未重导出** | 下游只能用自己 Cargo.toml 里的版本构造 →  silent 版本错配 |
| **公开依赖泄漏** | PR 里无意把私有依赖类型放进 pub fn → 用 `cargo-public-api` 等 CI 监控（§6） |

排查：**先 `cargo tree --duplicates`**（Item 25）看同名 crate 多版本。

---
