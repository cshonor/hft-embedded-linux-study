# 2.1 std::thread 生命周期

> 第 2 章 管理线程 · 下一节：[2.2 传参](02-passing-args.md)

## 这节讲什么

`std::thread` 的创建、`join`/`detach` 的区别，以及析构时仍 joinable 会 `terminate` 的致命坑。这是 C++ 多线程编程的第一个必须掌握的知识点。

## 为什么要学这个（先建立直觉）

C 程序员用 `pthread_create` 创建线程，用 `pthread_join` 等待。C++ 的 `std::thread` 在此基础上加了一层 RAII 语义——析构时检查状态，如果仍 joinable 就调用 `std::terminate` 拉崩进程。

```c
// C：pthread 裸句柄，忘了 join 也不会崩溃（但资源泄漏）
pthread_t t;
pthread_create(&t, NULL, func, NULL);
// 忘了 pthread_join → 线程资源泄漏，但程序不崩
```

```cpp
// C++：std::thread 析构检查 joinable
{
    std::thread t(func);
    // 忘了 join/detach → 析构时 terminate！
}  // ← 程序在这里崩溃
```

这个设计是**有意为之**——C++ 认为"创建了线程却不管它"是逻辑错误，应该尽早暴露。

## 核心操作详解

### 创建线程

```cpp
#include <thread>

// 1. 函数指针
void func(int x) { /* ... */ }
std::thread t1(func, 42);

// 2. lambda
std::thread t2([] { std::cout << "hello"; });

// 3. 成员函数
class Worker {
public:
    void run() { /* ... */ }
};
Worker w;
std::thread t3(&Worker::run, &w);  // 注意取地址 &w

// 4. 可调用对象
struct Functor {
    void operator()() { /* ... */ }
};
std::thread t4(Functor{});
```

### join vs detach

```cpp
std::thread t(func);

// join：阻塞当前线程，等 t 执行完毕
t.join();
// join 后 t 不再 joinable，可以安全析构

// 或者 detach：分离，t 在后台独立运行
// t.detach();
// detach 后 t 不再 joinable，失去对线程的控制
```

### 生命周期状态机

```
创建 ──→ joinable=true
  │
  ├── join()  ──→ joinable=false ──→ 安全析构 ✓
  ├── detach() ──→ joinable=false ──→ 安全析构 ✓
  └── (什么也不做) ──→ 析构时仍 joinable=true ──→ terminate! ✗
```

| 操作 | 效果 | joinable 之后 | 适用场景 |
|------|------|---------------|----------|
| `join()` | 阻塞等待完成 | false | 需要结果或确保完成 |
| `detach()` | 后台独立运行 | false | fire-and-forget |
| `std::move` | 转移所有权 | 原 false, 新 true | 存入容器/转移管理 |
| 析构(joinable) | **terminate** | — | 绝对避免！ |

## 常见错误（新手踩坑）

### 错误 1：异常路径忘记 join

```cpp
void buggy() {
    std::thread t(long_task);
    risky_operation();  // 可能抛异常
    t.join();           // 异常时永远到不了
}
// 异常展开栈 → t 析构 → terminate
```

**修复**：用 RAII 守卫或 try/catch。

```cpp
void safe() {
    std::thread t(long_task);
    try {
        risky_operation();
    } catch (...) {
        t.join();  // 异常路径也 join
        throw;
    }
    t.join();
}
```

### 错误 2：detach 后访问共享数据

```cpp
int result = 0;
void buggy() {
    std::thread t([&result] {
        result = compute();  // 访问外部变量
    });
    t.detach();  // 分离
}  // 函数返回，result 可能已销毁（如果是局部变量）

// 修复：不用 detach，或用 promise/future 传结果
```

### 错误 3：重复 join

```cpp
std::thread t(func);
t.join();
t.join();  // 错误！join 后不再 joinable，再次 join 是 UB
// join() 前应检查 joinable()
```

**修复**：`if (t.joinable()) t.join();`

## 和 C 的区别

| 特性 | C (pthread) | C++ (std::thread) |
|------|-------------|-------------------|
| 创建 | `pthread_create(&t, NULL, func, arg)` | `std::thread t(func, arg)` |
| 等待 | `pthread_join(t, NULL)` | `t.join()` |
| 分离 | `pthread_detach(t)` | `t.detach()` |
| 析构检查 | 无（忘了 join 不崩） | 有（joinable → terminate） |
| 句柄类型 | `pthread_t`（裸值） | `std::thread`（RAII 对象） |
| 可移动 | 不支持 | 支持 `std::move` |
| 传参 | `void*` 手动转型 | 模板推导，支持引用/移动 |

## HFT 关联

- **固定线程 + 绑核**：HFT 不在热路径创建/销毁线程（开销 + 抖动），启动时建固定线程池 + `pthread_setaffinity` 绑核。`std::thread` 主要用于启动阶段。
- **永不 detach**：HFT 守护进程里 detach 的线程失控——无法监控、无法优雅退出。所有线程都显式管理。
- **join 超时**：`std::thread::join` 不支持超时。HFT 需要超时等待时用 `std::condition_variable` 或 `std::future::wait_for`。

## 代码自测

### Q1: 下列代码会发生什么？

```cpp
void f() {
    std::thread t([] {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });
    // 没有 join 或 detach
}
int main() {
    f();
    std::cout << "done";
    return 0;
}
```

<details>
<summary>答案与复习指引</summary>

**`std::terminate` 拉崩进程**。`f()` 返回时 `t` 析构，此时仍 joinable（线程在 sleep 1 秒），析构调用 `std::terminate`。

"done" 不会被打印——程序在 `f()` 返回时崩溃。

复习：`std::thread` 析构时如果 `joinable() == true`，调用 `std::terminate`。每创建一个 thread，必须在所有路径（含异常）都 join 或 detach。
</details>

### Q2: 下列代码正确吗？

```cpp
std::thread t1(func);
std::thread t2 = std::move(t1);
t1.join();  // 安全吗？
```

<details>
<summary>答案与复习指引</summary>

**不安全**。`std::move(t1)` 后，`t1` 不再 joinable，调用 `t1.join()` 在非 joinable 线程上是 UB（可能抛 `system_error`）。

修复：移动后只对 `t2` 操作。或加检查：`if (t1.joinable()) t1.join();`

复习：移动语义转移所有权——原 thread 变成空状态（not joinable）。
</details>

### Q3: 如何安全地在异常路径管理线程？

```cpp
void worker() {
    std::thread t(long_task);
    try {
        do_work();  // 可能抛异常
    } catch (...) {
        // ???
    }
    t.join();
}
```

<details>
<summary>答案与复习指引</summary>

最简方案：catch 块中先 join 再 rethrow：

```cpp
} catch (...) {
    t.join();
    throw;
}
```

更好方案：用 RAII 守卫或 C++20 `std::jthread`，自动在析构时 join：

```cpp
std::jthread t(long_task);  // 析构自动 join，异常路径也安全
```

复习：RAII 是 C++ 管理资源的核心范式——构造获取，析构释放，所有路径安全。
</details>

### Q4: 为什么 HFT 不用 `std::thread::detach`？

<details>
<summary>答案与复习指引</summary>

三个原因：
1. **失控**：detach 后无法监控线程状态、无法优雅退出、无法获取异常。
2. **资源**：detached 线程的栈和资源在线程结束时才释放，无法控制时机。
3. **确定性**：detach 的线程何时结束不确定，影响关闭流程的确定性。

HFT 所有线程显式管理：启动时创建 + 注册，关闭时发停止信号 + join 等待。

复习：detach = "fire and forget"，只有在线程不需要返回结果且生命周期与进程一致时才可接受。HFT 不接受"失控"。
</details>

---

## 参考与延伸

- 下一节：[2.2 传参](02-passing-args.md)
- 回到：[第 2 章 管理线程](README.md)
