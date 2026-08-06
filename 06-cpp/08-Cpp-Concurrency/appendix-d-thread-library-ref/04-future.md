# D.4 future 异步结果

> 附录 D · 上一节：[D.3 atomic 原子操作](03-atomic.md) · 下一节：[D.5 condition_variable 条件变量](05-condvar.md)

## 这节讲什么

`<future>` 头文件提供异步结果传递设施。本节是速查参考——`future`/`promise`/`packaged_task`/`async` 的接口和用法。

---

## 核心规则（代码+表格）

### 四大组件

| 组件 | 作用 | 可移动 | 可拷贝 |
|------|------|--------|--------|
| `promise<T>` | 设置结果/异常 | 是 | 否 |
| `future<T>` | 获取结果/异常 | 是 | 否 |
| `shared_future<T>` | 共享 future | 是 | 是 |
| `packaged_task<F>` | 包装可调用对象 | 是 | 否 |
| `async(f, args...)` | 启动异步任务 | 返回 future | - |

### `promise` / `future` 配对

```cpp
std::promise<int> p;
std::future<int> f = p.get_future();

// 线程1：设置结果
std::thread t([&p]{
    int result = compute();
    p.set_value(result);  // 设置值
    // 或 p.set_exception(std::current_exception());  // 传递异常
});

// 线程2：获取结果
int v = f.get();  // 阻塞直到结果就绪
t.join();
```

### `async`：最简异步

```cpp
// 默认策略：async 或 deferred（实现决定）
auto f1 = std::async([]{ return 42; });

// 明确指定立即执行
auto f2 = std::async(std::launch::async, []{ return 42; });

// 延迟执行（get() 时才执行）
auto f3 = std::async(std::launch::deferred, []{ return 42; });

int v1 = f1.get();  // 阻塞等待
int v2 = f2.get();
int v3 = f3.get();  // 此时才执行 lambda
```

### `packaged_task`

```cpp
// 包装可调用对象，返回 future
std::packaged_task<int(int)> task([](int x){ return x * 2; });
std::future<int> f = task.get_future();

std::thread t(std::move(task), 21);  // task 可移动不可拷贝
t.detach();

int v = f.get();  // 42
```

### `shared_future`：多消费者

```cpp
std::promise<int> p;
std::shared_future<int> sf = p.get_future().share();
// sf 可以拷贝 → 多个消费者

std::thread t1([sf]{ std::cout << "t1: " << sf.get() << "\n"; });
std::thread t2([sf]{ std::cout << "t2: " << sf.get() << "\n"; });
// 两个线程都能 get()（普通 future 只能 get 一次）

p.set_value(42);
t1.join(); t2.join();
```

### 异常传递

```cpp
std::future<int> f = std::async(std::launch::async, []{
    throw std::runtime_error("oops");
    return 42;
});

try {
    int v = f.get();  // 重新抛出 runtime_error
} catch (const std::runtime_error& e) {
    std::cerr << "caught: " << e.what() << "\n";
}
```

### 超时等待

```cpp
auto f = std::async(std::launch::async, []{
    std::this_thread::sleep_for(1s);
    return 42;
});

// 非阻塞检查
if (f.wait_for(100ms) == std::future_status::ready) {
    int v = f.get();  // 就绪
} else {
    // 未就绪
}

// 等到时间点
f.wait_until(std::chrono::steady_clock::now() + 500ms);
```

### future 的局限

| 局限 | 说明 |
|------|------|
| 只能 get 一次 | 普通 future get 后失效（用 shared_future） |
| 不能组合 | 不能 "等任一 future" 或 "等所有 future" |
| 阻塞 get | get() 阻塞，无回调机制 |
| 无取消 | 不能取消 async 任务（除非 jthread + stop_token） |

```cpp
// C++20: wait_for_any / wait_for_all（提案，未标准）
// 目前需自己实现

// 等任一：用 promise + atomic 计数
std::atomic<int> done_count{0};
std::promise<int> first_done;
auto first_fut = first_done.get_future();

auto worker = [&](int result) {
    // ... work ...
    if (done_count.fetch_add(1) == 0) {
        first_done.set_value(result);  // 第一个完成的设置
    }
};
```

---

## 新手要点（和 C 的区别）

- **C 没有等价机制**：C 的异步结果传递要靠全局变量 + mutex/cv——不安全且繁琐。C++ 的 `future`/`promise` 是标准化的异步结果通道。
- **`async` 比 `std::thread` 更高层**：C 程序员可能习惯 `pthread_create` + 全局变量取结果——`std::async` 返回 future，一行代码搞定异步+取结果。
- **异常传递是 C++ 独有优势**：C 用错误码——不跨线程。C++ 的 `promise::set_exception` + `future::get()` 让异常安全跨线程传递。
- **`shared_future` 类似 `shared_ptr`**：C 程序员如果理解 `shared_ptr`，`shared_future` 类似——多个消费者共享同一个 future。普通 future 只能 get 一次。

---

## HFT 关联

- **HFT 热路径不用 future**：`future::get()` 阻塞，有同步开销——HFT 热路径用 SPSC 队列传递结果。
- **`async` 用于管理面**：HFT 的异步配置加载、日志刷盘用 `std::async`——简洁，返回 future 方便取结果。
- **异常传递用于策略计算**：策略线程计算可能失败——用 `promise::set_exception` 传回主线程，不让策略线程崩溃。
- **`shared_future` 用于多消费者**：如市场快照需要被多个策略线程读取——`shared_future<Snapshot>` 让多线程共享同一个快照。

---

## 自测题

1. `promise` 和 `future` 的关系是什么？如何配对使用？
2. `std::async` 的三种启动策略有什么区别？
3. `shared_future` 和普通 `future` 有什么区别？
4. 如何通过 `future` 传递异常？在哪个点重新抛出？
5. future 有哪些局限？为什么 HFT 热路径不用 future？

---

## 参考与延伸

- 下一节：[D.5 condition_variable 条件变量](05-condvar.md)
- 上一节：[D.3 atomic 原子操作](03-atomic.md)
- 回到：[附录 D](README.md)
