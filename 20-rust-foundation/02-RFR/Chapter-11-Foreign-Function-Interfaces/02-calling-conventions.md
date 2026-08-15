# 1.2 Calling Conventions（调用约定）

> 所属：**Crossing Boundaries with extern** · [← 章索引](./README.md)

## `extern "C"`

FFI **事实标准 ABI** — 参数与寄存器保存与 C 对齐。

## Panic / Unwind 边界

**禁止 unwind 穿过 FFI** — Rust panic 进入 C 侧 → **UB**（除非 crate 策略为 abort 且边界设计一致）。

## 实践

导出给 C 的 `extern "C"` 入口：

- **`catch_unwind`** + 错误码 / 哨兵
- 文档化**线程、重入**语义

→ [06 安全封装](./06-safety.md) · ER → [Item 34](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/README.md)
