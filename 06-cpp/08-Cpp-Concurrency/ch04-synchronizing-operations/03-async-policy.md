# 4.3 std::async 启动策略

> 第 4 章 · 上一节：[4.2 future/promise](02-future-promise.md) · 下一节：[4.4 future 的局限](04-future-limits.md)

## 这节讲什么

`async` 默认策略可能延迟到 `get()` 才同步执行——这是最容易被忽略的并发陷阱。要真正异步必须显式 `launch::async`。

---

## 三种策略

```cpp
auto f1 = std::async(std::launch::async, work);    // 立即新线程
auto f2 = std::async(std::launch::deferred, work);  // 延迟到 get() 才执行
auto f3 = std::async(work);                          // 默认：async | deferred
```

**默认策略的坑**：`async(fn)` 不保证立即执行，可能延后到 `f.get()` 才在同线程跑——破坏并发性。

---

## 新手要点

- **永远显式写 `launch::async`**：默认策略是陷阱——你以为异步，实际可能同步阻塞。
- **`launch::deferred` 的用途**：延迟计算（lazy evaluation）——但这是极少数场景。

---

## HFT 关联

- **热路径意外阻塞**：HFT 异步任务若用默认 `async` 策略，可能被延迟到 `get()` 同步执行，热路径意外阻塞。务必 `launch::async`。

---

## 自测题

1. `std::async` 默认启动策略的坑是什么？
2. `launch::async` 和 `launch::deferred` 分别是什么行为？
3. 为什么热路径要显式写 `launch::async`？

---

## 参考与延伸

- 下一节：[4.4 future 的局限](04-future-limits.md)
- 回到：[第 4 章](README.md)
