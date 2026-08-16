# Item 17 · 核心知识点

← [Item 17 目录](./README.md)

### 共享状态并行（Shared-state parallelism）

- 多线程通过**共享内存**通信。
- 经典风险：**数据竞争 (data races)**、**死锁 (deadlocks)**。

### 数据竞争三要素

1. 两线程访问**同一内存**
2. 至少一方**写入**
3. **无同步**规定顺序

→ 满足则 data race（C/C++ 中 UB；Rust **安全代码编译期杜绝**）。

### 死锁

- 线程互相等锁 → 永久停滞。
- **Rust 编译期无法防止死锁**。

### `Mutex<T>`

- 包装共享数据；`lock()` → **`MutexGuard`**（RAII，`Deref`/`DerefMut`，`Drop` 解锁）。

### 标记 trait

| Trait | 含义 |
|-------|------|
| **`Send`** | 所有权可**跨线程转移** |
| **`Sync`** | `&T` 可**跨线程共享** |

---
