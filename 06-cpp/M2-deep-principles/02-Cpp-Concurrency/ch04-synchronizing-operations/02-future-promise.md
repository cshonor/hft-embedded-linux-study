# 4.2 future / promise：一次性结果传递

> 第 4 章 · 上一节：[4.1 条件变量](01-condition-variable.md) · 下一节：[4.3 async 启动策略](03-async-policy.md)

## 这节讲什么

`promise` 是写入端，`future` 是读取端——一对一的结果传递通道。`get()` 阻塞直到 `set_value`。

---

## 核心用法

```cpp
std::promise<int> p;
std::future<int> f = p.get_future();
std::thread([&]{ p.set_value(42); }).detach();
int v = f.get();   // 阻塞直到 set_value
```

| 组件 | 角色 |
|------|------|
| `promise<T>` | 写入端（设值或异常） |
| `future<T>` | 读取端（阻塞 `get`） |
| `shared_future<T>` | 多读者共享同一个结果 |
| `packaged_task` | 包装可调用对象，自动绑 promise/future |

---

## 新手要点

- **`get()` 只能调一次**：`get()` 后 future 失效。多读者要用 `shared_future`。
- **异常传播**：`promise` 可以 `set_exception`，`future::get` 会 rethrow——异步任务的异常能传播到等待方。

---

## HFT 关联

- **避免在纳秒级热路径用**：promise/future 内部有共享状态 + atomic + 可能的堆分配，延迟不可控。热路径用 SPSC 队列 + 序列号更可控。

---

## 自测题

1. `promise` 和 `future` 的角色分别是什么？
2. `future::get()` 为什么只能调一次？多读者怎么办？
3. 异步任务的异常如何传播到等待方？

---

## 参考与延伸

- 下一节：[4.3 async 启动策略](03-async-policy.md)
- 回到：[第 4 章](README.md)
