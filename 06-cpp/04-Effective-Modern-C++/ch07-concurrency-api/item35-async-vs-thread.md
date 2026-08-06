# Item 35：优先 std::async 而非 std::thread

> 第 7 章 并发 API · Item 35 · 下一节：[Item 36 启动策略](item36-launch-policy.md)

## 这节讲什么

`std::thread` 是"手动管理线程"，`std::async` 是"声明并发任务，让运行时管线程"。`async` 返回 `future`，异常通过 future 传播（`thread` 里抛异常直接 `terminate`）。

---

## 对比

```cpp
// thread：手动管理
std::thread t([]{ return 42; });
// 无法直接拿返回值！异常会 terminate

// async：声明式
auto fut = std::async([]{ return 42; });
int result = fut.get();  // 拿返回值
// 异常通过 future 传播，get() 时 rethrow
```

---

## 新手要点（和 C 的区别）

- **C 用 pthread**：C 程序员习惯 `pthread_create` 手动管理线程。C++ 的 `async` 是更高层的抽象——让运行时管线程，你只管任务。
- **`async` 更安全**：异常通过 future 传播而非 `terminate`；返回值通过 `get()` 拿而非共享变量。
- **注意 Item 36**：`async` 默认策略可能延迟执行——要真正异步必须显式指定。

---

## HFT 关联

- **异步风控检查**：`auto fut = std::async(std::launch::async, checkRisk, order);` 异步执行风控，主线程不阻塞。

---

## 自测题

1. `std::async` 相比 `std::thread` 有什么优势？
2. `thread` 里抛异常会怎样？`async` 呢？
3. `async` 如何拿返回值？

---

## 参考与延伸

- 下一节：[Item 36 启动策略](item36-launch-policy.md)
- 回到：[第 7 章 并发 API](README.md)
