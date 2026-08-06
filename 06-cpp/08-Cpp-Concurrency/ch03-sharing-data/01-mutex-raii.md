# 3.1 mutex + RAII 锁

> 第 3 章 共享数据 · 下一节：[3.2 死锁及避免](02-deadlock.md)

## 这节讲什么

`mutex` 是保护共享数据的基本工具。`lock_guard`/`unique_lock` 的 RAII 保证异常安全——永远不要手写 `lock()`/`unlock()`。

---

## 核心用法

```cpp
std::mutex m;
std::lock_guard<std::mutex> lk(m);   // 构造加锁，析构解锁（RAII）
// 共享数据操作...
// 离开作用域自动解锁
```

`lock_guard`：最简 RAII 锁——构造即锁、析构即解。
`unique_lock`：更灵活——可延迟加锁（`defer_lock`）、可提前解锁（`unlock()`）、可转移所有权。代价略大。

---

## 新手要点

- **永远不要手写 `lock()`/`unlock()`**：异常路径会忘记解锁 → 死锁。用 RAII 锁自动管理。
- **锁要覆盖完整操作**：不是锁住单步，而是锁住整个逻辑操作（见 3.3 接口级竞争）。

---

## HFT 关联

- **热路径避锁**：mutex 有上下文切换 + 调度抖动风险，HFT 热路径用无锁结构（`atomic`/SPSC 队列）替代。

---

## 自测题

1. 为什么永远不要手写 `lock()`/`unlock()`？
2. `lock_guard` 和 `unique_lock` 的区别是什么？
3. HFT 热路径为什么避免 mutex？

---

## 参考与延伸

- 下一节：[3.2 死锁及避免](02-deadlock.md)
- 回到：[第 3 章](README.md)
