# 5.5 原子标志的最简同步

> 第 5 章 · 上一节：[5.4 CAS](04-cas.md) · 下一节：[5.6 volatile ≠ atomic](06-volatile-not-atomic.md)

## 这节讲什么

`atomic<bool>` + release/acquire 是最简单的跨线程同步模式——一个线程写数据+置标志，另一个线程读标志+读数据。

---

## 核心模式

```cpp
std::atomic<bool> ready{false};
// 线程 A
data = 42;
ready.store(true, std::memory_order_release);   // release：之前的写对读到者可见

// 线程 B
while (!ready.load(std::memory_order_acquire));  // acquire：之后的读看到 release 前的写
assert(data == 42);   // 一定成立：release-acquire 建立了 happens-before
```

**关键**：`release` store 之前的所有写（`data = 42`）对 `acquire` load 之后的读可见。这就是 release-acquire 建立 happens-before 的最简形式。

---

## 新手要点

- **这是无锁同步的基础**：不用 mutex，用 `atomic<bool>` + release/acquire 就能安全传递数据。
- **内存序不能省**：如果用 `relaxed`，`data = 42` 和 `ready = true` 可能被重排——消费者可能看到 `ready=true` 但 `data` 还是旧值。

---

## HFT 关联

- **SPSC 队列的基础**：生产者写数据 + release 存序列号；消费者 acquire 读序列号后读数据——HFT SPSC 无锁队列的经典模式。

---

## 自测题

1. release-acquire 如何用 `atomic<bool>` 建立跨线程同步？
2. 如果用 `relaxed` 替代 release/acquire，会有什么问题？
3. 这个模式为什么不需要 mutex？

---

## 参考与延伸

- 下一节：[5.6 volatile ≠ atomic](06-volatile-not-atomic.md)
- 回到：[第 5 章](README.md)
