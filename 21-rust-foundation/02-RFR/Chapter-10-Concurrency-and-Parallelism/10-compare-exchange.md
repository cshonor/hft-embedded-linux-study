# 4.4 Compare and Exchange（CAS）

> 所属：**Lower-Level Concurrency** · [← 章索引](./README.md)

**Compare-And-Swap** — 无锁算法核心原语。

```rust
loop {
    let cur = atomic.load(Ordering::Acquire);
    if atomic.compare_exchange_weak(cur, new, Ordering::AcqRel, Ordering::Acquire).is_ok() {
        break;
    }
}
```

## `compare_exchange_weak`

ARM 等上允许**伪失败**（无竞争也可能失败）→ **loop 重试**；部分平台代码更优。

→ [11 Fetch 方法](./11-fetch-methods.md)
