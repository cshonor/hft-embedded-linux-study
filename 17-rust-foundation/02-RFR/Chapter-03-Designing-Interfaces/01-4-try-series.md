# 1.1.4 · `try_` 系列 — 错误处理

> 所属：**Unsurprising / 命名惯例** · [← 01 命名 hub](./01-naming-practices.md)

← [01-3 `get_`](./01-3-get-series.md) · 下一节 [01-5 `with_`](./01-5-with-series.md)

Demo → `cargo run --manifest-path naming-series-demo/Cargo.toml try`

---

## 核心逻辑

| 点 | 说明 |
|----|------|
| **对标** | 「必然成功」版本的**可失败**版 |
| **失败时** | 返回 **`Err`**，**不 panic**（多数 API 也不阻塞） |
| **所有权** | 通常**不消耗** `self` |
| **口诀** | 可能失败 → `Result` |

```text
lock()      ──忙──► 阻塞等待
try_lock()  ──忙──► Err（立即返回）

Into::into()     ──失败──► 编译期须保证成功
TryInto::try_into() ──失败──► Err
```

| 常见 API | 对比 | 失败 |
|----------|------|------|
| **`try_lock()`** | `lock()` 阻塞 | 拿不到锁 → `Err` |
| **`try_into()`** | `into()` | 转换失败 → `Err` |
| **`try_reserve()`** | `reserve()` | 分配失败 → `Err` |

---

## 可运行示例

### `try_lock` — 持锁时再试 → `Err`

```rust
use std::sync::Mutex;

let lock = Mutex::new(5);
let _guard = lock.lock().unwrap();
assert!(lock.try_lock().is_err()); // 锁被持有 → Err，不阻塞
```

### 释放锁后 → `Ok`

```rust
use std::sync::Mutex;

let lock = Mutex::new(5);
{
    let mut guard = lock.lock().unwrap();
    *guard = 10;
} // guard drop，锁释放
let guard2 = lock.try_lock().expect("锁应空闲");
assert_eq!(*guard2, 10);
```

### `try_into` — 带错误的转换

```rust
let n: i32 = 42;
let big: i64 = n.try_into().unwrap();

let huge: i128 = 1_000_000_000_000;
let small: i8 = huge.try_into(); // Err — 溢出
assert!(small.is_err());
```

→ demo：`naming-series-demo/src/try_series.rs`

---

## 设计反例

该 `try_` 却 **panic** → 调用方无法用 `?` / `match` 处理，破坏 Unsurprising 直觉。

---

## 速记

**口诀**：可失败 → `Result` · 通常不消耗 self · `lock()` 阻塞 · `try_lock()` → `Err`

→ 下一节：[01-5 `with_`](./01-5-with-series.md)
