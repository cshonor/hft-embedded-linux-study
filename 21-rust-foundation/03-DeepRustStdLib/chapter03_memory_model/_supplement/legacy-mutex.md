# 3.6 `Mutex`

> 章索引：[第 3 章](./README.md) · 前：[3.5 RefCell](./3.5-refcell.md) · 后：[3.7 RwLock](./3.7-rwlock.md)

---

## 一句话

**`Mutex<T>`** = **互斥锁 + 内部可变**：任意时刻最多一个线程持有 **`MutexGuard<T>`**（类似 `&mut T`）— **`T` 不要求 `Sync`**，但 **`Mutex<T>: Sync`** 当 `T: Send` 时，故 **`Arc<Mutex<T>>`** 是跨线程共享可变的最常见模式。

---

## 与 `RefCell` 的分工（勿合并）

| | **`RefCell`** | **`Mutex`** |
|--|---------------|-------------|
| **检查时机** | 编译期 + 运行时（单线程） | 阻塞锁（跨线程） |
| **`Send`/`Sync`** | 非 Sync | `T: Send` → `Mutex<T>: Sync` |
| **失败** | `borrow_mut` panic | 死锁 / poison（panic 后） |
| **场景** | 单线程图、UI 状态 | 行情缓存、订单簿、日志队列 |

```rust
use std::sync::{Arc, Mutex};

let data = Arc::new(Mutex::new(Vec::new()));
let d = Arc::clone(&data);
std::thread::spawn(move || {
    d.lock().unwrap().push(1);
});
```

---

## 标准库细节

- **`lock()`** — 阻塞至获得锁；`try_lock` 非阻塞。
- **Poison** — 持锁线程 panic 后，锁标记为 poisoned，`lock` 得 `Err`。
- 底层 OS 原语因平台而异（pthread、Windows SRW 等）。

→ [05-atomic Ch1 mutex](../../05-Async-Concurrency-Network/01-atomic/Chapter-01-Rust-Concurrency-Basics/1.7-mutex-rwlock/1.7-mutex-rwlock.md) · [RFR Ch10](../../02-RFR/Chapter-10-Concurrency-and-Parallelism/README.md)

---

## HFT 提示

低延迟路径常 **避免全局 `Mutex`**（阻塞、优先级反转）；倾向 **无锁 / 分片 / 每线程队列** — 但读 `std::sync` 源码仍是理解 **happens-before** 的基线。

---

## 相关

- [3.7 RwLock](./3.7-rwlock.md) — 多读单写
- [3.5 RefCell](./3.5-refcell.md)
