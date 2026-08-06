# 4.4 future 的局限

> 第 4 章 · 上一节：[4.3 async 启动策略](03-async-policy.md) · 下一节：[4.5 超时等待](05-timeout.md)

## 这节讲什么

future 有三个局限：只能取一次、不能轮询、析构阻塞（async 返回的 future）。

---

## 三大局限

1. **只能取一次**：`get()` 后 future 失效，不能重复等。多读者要用 `shared_future`。
2. **不能轮询**：标准库没有 `try_get`，要么阻塞 `get()`，要么 `wait_for`/`wait_until` 超时。
3. **析构阻塞**：`async` 返回的 future 在析构时会阻塞等待线程结束——隐式同步行为。

```cpp
{
    auto f = std::async(std::launch::async, work);
    // ... 如果不 get ...
}  // f 析构 → 阻塞等 work 完成！
```

---

## 新手要点

- **析构阻塞最反直觉**：你以为不调 `get()` 就不等待，但 `async` 返回的 future 析构会隐式 join。
- **避免析构阻塞**：把 future 存到后台线程，或用 `shared_future`。

---

## HFT 关联

- **future 析构阻塞陷阱**：`async` 返回的 future 析构会 join，热路径上误用会导致隐式串行化。

---

## 自测题

1. `future::get()` 为什么只能调一次？
2. `async` 返回的 future 析构时会发生什么？
3. 为什么 future 不能轮询？

---

## 参考与延伸

- 下一节：[4.5 超时等待](05-timeout.md)
- 回到：[第 4 章](README.md)
