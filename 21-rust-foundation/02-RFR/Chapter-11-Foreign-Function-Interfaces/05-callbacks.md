# 2.3 Callbacks（回调）

> 所属：**Types Across Language Boundaries** · [← 章索引](./README.md)

C 常通过 **函数指针** 回调 Rust。

## Rust → C

```rust
type Callback = extern "C" fn(i32) -> i32;
extern "C" { fn register(cb: Callback); }
```

## C → Rust

- 导出 **`extern "C" fn`** 给 C 注册
- **`catch_unwind`** — 回调内 panic 不得穿越 C

## `Send` / 生命周期

- 回调若跨线程 / 异步存储 → 明确 **`Send`** 与**存活期**（C 何时不再调用）
- 避免 **`'static` 闭包**误用栈上数据

→ [05 安全](./06-safety.md) · [02 调用约定](./02-calling-conventions.md)
