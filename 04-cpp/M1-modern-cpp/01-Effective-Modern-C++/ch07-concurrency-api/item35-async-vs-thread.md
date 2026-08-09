# Item 35：优先 std::async 而非 std::thread

> 第 7 章 并发 API · Item 35 · 下一节：[Item 36 启动策略](item36-launch-policy.md)

## 为什么要学这个（先建立直觉）

C 程序员用 `pthread` 手动管理线程：

```c
#include <pthread.h>

void* worker(void* arg) {
    int result = compute();
    return (void*)(intptr_t)result;
}

pthread_t tid;
pthread_create(&tid, NULL, worker, NULL);
// ... 无法直接拿返回值 ...
void* retval;
pthread_join(tid, &retval);
int result = (int)(intptr_t)retval;
// 异常？pthread 里没有异常——如果 worker 崩溃，进程崩溃
```

C++ 有 `std::thread`，但和 `pthread` 一样是"手动管理"——拿不到返回值，异常会导致 `terminate`。`std::async` 是更高层的抽象：

```cpp
auto fut = std::async([]{ return compute(); });
int result = fut.get();  // 拿返回值
// 异常通过 future 传播，get() 时 rethrow——不会 terminate
```

---

## 这节讲什么

`std::thread` 是"手动管理线程"，`std::async` 是"声明并发任务，让运行时管线程"。`async` 返回 `future`，异常通过 future 传播（`thread` 里抛异常直接 `terminate`）。

---

## 核心对比

### thread：手动管理

```cpp
std::thread t([]{ return 42; });
// 无法直接拿返回值！
// 异常会 terminate
t.join();  // 必须手动 join
```

### async：声明式

```cpp
auto fut = std::async([]{ return 42; });
int result = fut.get();  // 拿返回值
// 异常通过 future 传播，get() 时 rethrow

// 线程管理交给运行时——不需要手动 join
// 运行时可能用线程池，避免频繁创建/销毁线程
```

### 异常处理对比

```cpp
// thread：异常 = 灾难
std::thread t([]{
    throw std::runtime_error("oops");
});
// 异常未捕获 → std::terminate → 进程崩溃
t.join();

// async：异常安全
auto fut = std::async([]{
    throw std::runtime_error("oops");
});
try {
    fut.get();  // 在这里 rethrow 异常
} catch (const std::runtime_error& e) {
    std::cerr << e.what() << "\n";  // 优雅处理
}
```

---

## 常见错误（新手踩坑）

**错误 1：thread 里抛异常导致 terminate**
```cpp
std::thread t([]{
    throw std::runtime_error("error");  // 进程崩溃！
});
```
**修正：** 用 `std::async`，异常通过 `future` 传播。

**错误 2：thread 拿不到返回值**
```cpp
std::thread t([]{ return 42; });
// 结果丢失——thread 没有返回值机制
```
**修正：** 用 `std::async` + `future::get()`。

**错误 3：忘了 fut.get() 导致 future 析构阻塞**
```cpp
{
    auto fut = std::async(std::launch::async, []{ long_task(); });
    // fut 析构时如果任务还没完成 → 阻塞等待
}  // 这里会卡住直到 long_task 完成
```
**修正：** 理解 `future` 析构会等待（Item 38），或用 `std::launch::deferred`。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 线程创建 | `pthread_create` | `std::thread` / `std::async` | C++ 标准库 |
| 返回值 | `pthread_join` + `void*` | `future::get()` | 类型安全 |
| 异常 | 进程崩溃 | `future` 传播 | 异常安全 |
| 线程管理 | 手动 | `async` 自动 | 更高层抽象 |

**一句话总结：** C 程序员记住——`std::async` 是 `pthread_create` 的高层替代：返回值通过 `future` 拿，异常通过 `future` 传，线程管理交给运行时。

---

## HFT 关联

- **异步风控检查**：`auto fut = std::async(std::launch::async, checkRisk, order);` 异步执行风控，主线程不阻塞。
- **后台日志**：`std::async(std::launch::async, []{ write_log(entries); });` 异步写日志，不阻塞热路径。
- **异常安全**：HFT 守护进程用 `async` 而非 `thread`——任务抛异常不会拉崩进程。

---

## 自测题

1. `std::async` 相比 `std::thread` 有什么优势？
2. `thread` 里抛异常会怎样？`async` 呢？
3. `async` 如何拿返回值？
4. 下面代码有什么问题？
```cpp
std::thread t([]{ return compute(); });
t.join();
// 怎么拿 compute() 的返回值？
```

---

## 参考与延伸

- 下一节：[Item 36 启动策略](item36-launch-policy.md)
- 回到：[第 7 章 并发 API](README.md)
