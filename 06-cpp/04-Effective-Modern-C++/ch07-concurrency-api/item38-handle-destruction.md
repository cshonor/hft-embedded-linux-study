# Item 38：理解线程句柄的析构与联结行为

> 第 7 章 · Item 38 · 上一节：[Item 37 thread joinable](item37-thread-joinable.md)

## 这节讲什么

不同线程句柄的析构行为不同——`thread` 析构会 `terminate`，`future` 析构会阻塞等待。

---

## 析构行为对比

| 句柄类型 | 析构行为 |
|----------|----------|
| `std::thread` | joinable 则 `terminate` |
| `std::future` | 阻塞等待（共享状态未就绪时）——析构会等任务完成 |
| `std::shared_future` | 不阻塞（多个 shared_future 共享状态） |

`future` 析构会阻塞——反直觉但有用：让你"忘了 `get` 也能拿到结果"。但也可能让程序在意外处卡住。

---

## 新手要点

- **`future` 析构会阻塞**：这和 `thread` 析构 `terminate` 完全不同。记住——`thread` = 炸，`future` = 等。
- **别在热路径析构 future**：析构会等任务完成，热路径析构 future = 意外阻塞。

---

## HFT 关联

- **异步风控**：异步风控任务用 `future`，但确保不在热路径析构 future——把 future 存到后台线程或用 `shared_future`。

---

## 自测题

1. `std::thread` 和 `std::future` 的析构行为有何不同？
2. 为什么 `future` 析构会阻塞？这有什么用？
3. `shared_future` 析构为什么不阻塞？

---

## 参考与延伸

- 下一节：[Item 39 单次事件](item39-one-shot-events.md)
- 回到：[第 7 章](README.md)
