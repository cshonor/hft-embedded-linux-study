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

<details>
<summary>参考答案</summary>

1. 三点主要优势：①**能拿返回值**：`std::async` 返回 `std::future`，可以直接 `get()` 拿到任务的返回值（也能传异常）；`std::thread` 没有任何返回值通道，只能靠 `std::promise` 或输出参数自己搭。②**异常安全**：任务里抛的异常会被捕获并存入 future 的共享状态，在 `get()` 时重新抛出，由调用方处理；`std::thread` 里逃逸的异常直接调用 `std::terminate`，进程崩溃。③**生命周期更简单**：`std::thread` 析构时若仍 joinable 会 `terminate`，必须自己 join/detach（或用 RAII 封装）；`async` 返回的 future 由库管理底层线程，不必手动 join。另外 `async` 还能让运行时选择线程池等实现。

2. `std::thread` 里抛出的异常若没被线程函数捕获，会调用 `std::terminate()`——整个进程直接终止（栈展开不会跨线程传播，没有其他线程能 catch 到它）。`std::async` 里抛的异常被库捕获并存储到 future 的共享状态中，等到调用 `future::get()`/`wait()` 时在**调用方线程**重新抛出，可以正常 try-catch 处理；如果没人调用 `get()`，异常就被静默丢弃（这也是要记得取结果的原因）。

3. 通过返回的 `std::future<T>`：`auto fut = std::async(std::launch::async, f, args...);`，之后 `T r = fut.get();`。`get()` 会阻塞直到任务完成（若未完成），并**只能调用一次**（它是移动语义，调用后 future 失效）；可用 `wait()`/`wait_for()` 做非阻塞或超时等待。若不需要返回值，`async` 返回的 `std::future<void>` 仍要用 `get()` 来同步与传播异常。

4. 拿不到——`std::thread` 没有返回值的机制，lambda 的返回值被直接丢弃（甚至 `[]{ return compute(); }` 这种写法本身没有意义）。可行的改法有两类：①改用 `std::async`，直接 `auto fut = std::async(std::launch::async, compute);` 然后 `fut.get()`；②坚持用 `thread` 则自己搭通道，例如 `std::promise<T> p; auto fut = p.get_future();` 在线程里 `p.set_value(compute());`，或用 `std::packaged_task` 包住可调用对象再 `get_future()`。异常路径下还要记得 `set_exception`。推荐第 ① 种，代码最短且异常安全。

</details>

---

## 参考与延伸

- 下一节：[Item 36 启动策略](item36-launch-policy.md)
- 回到：[第 7 章 并发 API](README.md)
