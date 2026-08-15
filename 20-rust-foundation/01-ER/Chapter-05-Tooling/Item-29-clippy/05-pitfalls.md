# Item 29 · 易错细节

← [Item 29 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **破窗效应** | 几条旧 warning 常驻 → 全员无视 Clippy |
| **crate 级 `#![allow(clippy::all)]`** | 等于关掉工具；几乎 never 合理 |
| **本地不跑、只靠 CI** | 反馈太晚；开发时 `cargo clippy` 应与 `cargo check` 同级习惯 |
| **升级 toolchain 后新 lint** | 零基线才能第一时间发现 |

---
