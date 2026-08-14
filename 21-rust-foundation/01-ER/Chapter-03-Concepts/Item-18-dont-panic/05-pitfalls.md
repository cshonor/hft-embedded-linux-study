# Item 18 · 易错细节

← [Item 18 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **`catch_unwind` 当异常** | abort 模式、FFI、状态撕裂三重失效 |
| **库内 `unwrap`** | 把可恢复错误变成线程/进程崩溃 |
| **FFI 边界** | Rust panic **不得**传播到 C 等调用方 → UB；须 `catch_unwind` + 转错误码或 abort 策略（见 Item 34） |
| **持锁 panic** | [锁中毒](../Item-17-shared-state-parallelism/README.md) — 另一路「panic 的代价」 |

---
