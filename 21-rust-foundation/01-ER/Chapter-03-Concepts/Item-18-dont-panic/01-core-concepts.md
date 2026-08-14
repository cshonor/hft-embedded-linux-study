# Item 18 · 核心知识点

← [Item 18 目录](./README.md)

### `panic!` 的定位

- 面向**不可恢复的 bug**（契约/不变量被破坏）。
- 默认：**终止当前线程**（可配置为 abort 整个进程）。

### `catch_unwind` 的局限

- 标准库提供 `std::panic::catch_unwind`，**不是** Rust 版的 `try-catch`。
- 用其「恢复业务错误」是**反模式**（见 §2）。

### Abort vs Unwind

| 模式 | 行为 |
|------|------|
| **unwind**（默认多数 target） | 沿栈展开，跑 `Drop` |
| **abort** | `Cargo.toml` `panic = "abort"` 或部分 target（如 WASM）→ **直接终止**，`catch_unwind` 无效 |

### 异常安全性（Exception safety）

- panic 发生在**数据结构更新中途** → 可能处于**不一致**状态。
- 在 panic 可能发生的上下文里维持不变量**极难** → 别指望 `catch_unwind` 当通用恢复手段。

---
