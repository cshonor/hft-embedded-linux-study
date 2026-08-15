# Item 31 · 易错细节

← [Item 31 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **废弃的 cargo 插件** | 看 crates.io 更新日期、Rust 版本兼容 |
| **企业 / 嵌入式定制链** | 无 `rustup`、无原生 `cargo`（如 Android Soong）→ 工具链与文档不同 |
| **只本地用不进 CI** | fmt/clippy 未 CI 化 → 主分支仍可 merge 脏代码 |
| **堆太多慢工具进 PR** | fuzz / 全 feature powerset 应定时 job（Item 30） |

---
