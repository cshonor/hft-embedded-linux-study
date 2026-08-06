# 8.4 异常安全

> 第 8 章 · 上一节：[8.3 减少共享：线程局部化](03-thread-local.md) · 下一节：[8.5 可扩展性法则](05-scalability.md)

## 这节讲什么

并发代码的异常安全比串行代码更复杂——异常传播跨越线程边界、`std::thread` 析构时未 join 会 `terminate`、future 的异常要靠 `future::get()` 重新抛出。本节讲这些陷阱及正确处理方式。

---

## 核心规则（代码+表格）

### `std::thread` 析构陷阱

```cpp
void dangerous() {
    std::thread t([]{ do_work(); });
    // 如果 do_work 抛异常，t 的析构函数被调用
    // t 未 join 也未 detach → std::terminate()！整个程序挂
}

// 正解1：RAII 守卫
void safe_raii() {
    std::thread t([]{ do_work(); });
    thread_guard g(t);  // 析构时 join，即使抛异常也 join
}

// 正解2：catch 后 join
void safe_catch() {
    std::thread t([]{ do_work(); });
    try {
        do_other_work();
    } catch (...) {
        t.join();  // 确保异常路径也 join
        throw;
    }
    t.join();
}
```

### 线程函数内的异常

```cpp
// 线程函数抛异常 → std::terminate（默认）
std::thread t([]{
    throw std::runtime_error("oops");  // 未捕获 → terminate
});

// 正解：线程内 catch 所有异常
std::thread t([]{
    try {
        do_work();
    } catch (const std::exception& e) {
        log_error(e.what());  // 记录但不传播
    } catch (...) {
        log_error("unknown error");
    }
});
```

### `std::async` 与 future 的异常传递

```cpp
// async 的异常会存入 future，get() 时重新抛出
auto fut = std::async(std::launch::async, []{
    throw std::runtime_error("worker error");
    return 42;
});

try {
    int result = fut.get();  // 这里重新抛出 runtime_error
} catch (const std::runtime_error& e) {
    std::cerr << "caught: " << e.what() << "\n";
}
// 这是 async/promise 优于 raw thread 的地方——异常可以跨线程传递
```

### `promise` 手动传递异常

```cpp
std::promise<int> p;
std::future<int> f = p.get_future();

std::thread t([&p]{
    try {
        int result = risky_computation();
        p.set_value(result);
    } catch (...) {
        p.set_exception(std::current_exception());  // 传递异常
    }
});

try {
    int v = f.get();  // 如果 set_exception 了，这里重新抛
} catch (const std::exception& e) {
    std::cerr << "caught: " << e.what() << "\n";
}
t.join();
```

### 异常安全保证等级（并发版）

| 保证 | 含义 | 并发额外要求 |
|------|------|-------------|
| 基本保证 | 异常后无泄漏、无损坏 | 锁不能死锁、不能 double-unlock |
| 强保证 | 操作成功或回滚 | 需要锁的回滚（极难） |
| 不抛保证 (`noexcept`) | 不抛异常 | 线程函数应尽量 noexcept |

---

## 新手要点（和 C 的区别）

- **C 没有异常机制**：C 程序员习惯用返回值错误码——错误码不会跨线程"传播"，调用方主动检查。C++ 异常可以跨线程（通过 future），但 raw thread 内未捕获异常会 `terminate`——这是 C 程序员转型 C++ 并发时最容易踩的坑。
- **`std::terminate` 是灾难**：C 程序员可能觉得"线程崩了就崩了"——但 C++ 的 `std::terminate` 会调用 `std::terminate_handler`（默认 `abort`），整个进程退出，连析构都不做。HFT 系统必须在线程函数内 catch 所有异常。
- **future 的异常传递是 C++ 独有优势**：C 里要手动传递错误码或用全局 `errno`——不跨线程。C++ 的 `promise::set_exception` + `future::get()` 让异常安全地跨线程传递，这是 async/promise 优于 raw thread 的重要原因。
- **`noexcept` 在并发中的价值**：C 没有 `noexcept`。C++ 的 `noexcept` 承诺不抛异常——线程函数标 `noexcept` 可以让编译器优化，也让调用者放心。但注意：`noexcept` 函数如果抛异常会直接 `terminate`。

---

## HFT 关联

- **HFT 系统绝不允许 `std::terminate`**：一条异常未捕获导致整个交易系统崩溃——这是灾难。所有线程函数必须有顶层 try-catch。
- **future 异常传递用于策略计算**：策略线程计算可能失败（如数据异常），用 `promise::set_exception` 把异常传回主线程处理——而不是让策略线程崩溃。
- **`noexcept` 标注热路径**：HFT 热路径的函数（行情解析、策略计算）标 `noexcept`——既是文档（承诺不抛），也允许编译器省略异常处理代码。
- **异常处理的性能代价**：C++ 异常在抛出路径有微秒级开销（table-based EH 在不抛时零开销，但抛出时慢）。HFT 热路径应避免抛异常——用错误码或 `std::expected`（C++23）。异常只用于真正"异常"的情况。

---

## 自测题

1. `std::thread` 析构时如果未 join 也未 detach，会发生什么？
2. 线程函数内抛出未捕获的异常会怎样？应该怎么处理？
3. `std::async` 返回的 future 如何传递异常？在哪个调用点重新抛出？
4. `promise::set_exception` 和 `future::get()` 如何配合传递异常？
5. 为什么 HFT 系统的线程函数必须有顶层 try-catch？

---

## 参考与延伸

- 下一节：[8.5 可扩展性法则](05-scalability.md)
- 上一节：[8.3 减少共享：线程局部化](03-thread-local.md)
- 回到：[第 8 章](README.md)
