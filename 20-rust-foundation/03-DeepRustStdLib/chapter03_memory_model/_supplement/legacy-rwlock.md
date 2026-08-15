# 3.7 `RwLock`

> 章索引：[第 3 章](./README.md) · 前：[3.6 Mutex](./3.6-mutex.md) · 后：[3.8 PhantomData](./3.8-phantomdata.md)

---

## 一句话

**`RwLock<T>`** = **读写锁**：**多个读** 或 **一个写** 二选一 — 读多写少时比 `Mutex` 并发度更高；API 类似 **`read()` / `write()`** 返回守卫。

---

## 与 `Mutex` 的分工（勿合并）

| | **`Mutex<T>`** | **`RwLock<T>`** |
|--|----------------|-----------------|
| **并发模式** | 任意时刻 1 个访问者 | 多 `read` **或** 1 `write` |
| **适用** | 读写频率接近、临界区短 | **读远多于写**（配置、行情快照） |
| **风险** | 实现简单 | 写饥饿、实现差异（平台） |
| **HFT** | 通用默认 | 只读热路径多时可评估；注意锁升级不存在 |

```rust
use std::sync::RwLock;

let lock = RwLock::new(0);
{
    let r = lock.read().unwrap();
    println!("{r}");
}
{
    let mut w = lock.write().unwrap();
    *w += 1;
}
```

---

## 标准库与陷阱

- **不能** 持有 `read` 再 `write`（死锁风险）— 与 `RefCell` 运行时 borrow 规则类似。
- **`std::sync::RwLock`** vs 生态 **`parking_lot::RwLock`** — 后者常更快，但语义以 std 为基准学习。

→ [RwLock 贯通笔记](../../05-Async-Concurrency-Network/01-atomic/RwLock与读写锁体系-贯通笔记.md)

---

## 相关

- [3.6 Mutex](./3.6-mutex.md)
- [3.9 泄漏与循环引用](./3.9-leaks-and-cycles.md) — `Arc<RwLock<_>>` 环
