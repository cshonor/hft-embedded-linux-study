# Item 36：明确指定启动策略

> 第 7 章 · Item 36 · 上一节：[Item 35 async vs thread](item35-async-vs-thread.md)

## 这节讲什么

`async` 默认策略是 `async | deferred`——运行时可能延迟到 `get()` 才同步执行，这违背"并发"初衷。

---

## 核心问题

```cpp
auto fut = std::async([]{ ... });  // 默认策略：async | deferred
// 可能延迟到 fut.get() 才同步执行！你以为异步，实际同步阻塞
```

要真正异步，显式指定：
```cpp
auto fut = std::async(std::launch::async, []{ ... });
// 强制新线程或线程池执行
```

| 策略 | 行为 |
|------|------|
| `launch::async` | 强制新线程/线程池执行 |
| `launch::deferred` | 延迟到 `get()`/`wait()` 才同步执行 |
| 默认（`async \| deferred`） | 运行时决定，可能是 deferred |

---

## 新手要点

- **默认策略是陷阱**：你以为异步，实际可能同步。**永远显式写 `launch::async`**。
- **`deferred` 的用途**：延迟计算（lazy evaluation）——但这是极少数场景，新手不用管。

---

## HFT 关联

- **热路径意外阻塞**：HFT 异步任务若用默认 `async` 策略，可能被延迟到 `get()` 同步执行，热路径意外阻塞。务必 `launch::async`。

---

## 自测题

1. `std::async` 的默认启动策略是什么？为什么可能导致"以为是异步实际是同步"？
2. `launch::async` 和 `launch::deferred` 分别是什么行为？
3. 为什么 HFT 必须显式指定 `launch::async`？

---

## 参考与延伸

- 下一节：[Item 37 thread joinable](item37-thread-joinable.md)
- 回到：[第 7 章](README.md)
