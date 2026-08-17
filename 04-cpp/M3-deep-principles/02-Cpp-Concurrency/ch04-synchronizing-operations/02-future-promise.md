# 4.2 future / promise：一次性结果传递

> 第 4 章 · 上一节：[4.1 条件变量](01-condition-variable.md) · 下一节：[4.3 async 启动策略](03-async-policy.md)

## 这节讲什么

`promise` 是写入端，`future` 是读取端——一对一的结果传递通道。`get()` 阻塞直到 `set_value`。这是 C++ 线程间传递结果的标准机制。

## 为什么要学这个（先建立直觉）

C 程序员用共享变量 + mutex/cv 传结果：

```c
// C：手动用 mutex + cv 传递结果
typedef struct {
    int result;
    int done;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
} ResultChannel;

void producer(ResultChannel* ch) {
    int val = compute();
    pthread_mutex_lock(&ch->mtx);
    ch->result = val;
    ch->done = 1;
    pthread_cond_signal(&ch->cv);
    pthread_mutex_unlock(&ch->mtx);
}

void consumer(ResultChannel* ch) {
    pthread_mutex_lock(&ch->mtx);
    while (!ch->done)
        pthread_cond_wait(&ch->cv, &ch->mtx);
    printf("%d\n", ch->result);
    pthread_mutex_unlock(&ch->mtx);
}
// 大量样板代码
```

C++ 用 `promise`/`future` 把这套模式封装成一行：

```cpp
// C++：promise/future 一对一结果传递
std::promise<int> p;
std::future<int> f = p.get_future();

// 生产者
std::thread([&p] {
    p.set_value(compute());  // 设置结果
}).detach();

// 消费者
int result = f.get();  // 阻塞等待结果
```

## 核心用法详解

### 基本流程

```cpp
#include <future>

// 1. 创建 promise
std::promise<int> p;

// 2. 获取 future（一对一，只能调一次）
std::future<int> f = p.get_future();

// 3. 生产者设值
std::thread producer([&p] {
    try {
        int result = long_computation();
        p.set_value(result);  // 正常设值
    } catch (...) {
        p.set_exception(std::current_exception());  // 传播异常
    }
});

// 4. 消费者取值
int value = f.get();  // 阻塞直到 set_value
// get() 只能调一次！第二次是 UB

producer.join();
```

### shared_future：多读者

```cpp
// future::get() 只能调一次——多读者用 shared_future
std::promise<int> p;
std::future<int> f = p.get_future();
std::shared_future<int> sf = f.share();  // 转为共享

// 多个线程可以 get 同一个结果
std::thread t1([&sf] { std::cout << sf.get(); });
std::thread t2([&sf] { std::cout << sf.get(); });
// 两个线程都能正确读到结果
```

### packaged_task：包装可调用对象

```cpp
// packaged_task 自动绑定 promise/future
std::packaged_task<int(int)> task([](int x) { return x * 2; });
std::future<int> f = task.get_future();

std::thread t(std::move(task), 42);
t.join();
std::cout << f.get();  // 84
```

### 组件关系

```
┌─────────────────────────────────────────────────┐
│  promise<T>  ←──写入──  共享状态  ──读取──  future<T>  │
│  (生产者)                    (一对一)           (消费者)  │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│  packaged_task<F>  ←─自动包装─→  promise + future  │
│  (包装可调用对象)                                   │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│  std::async(policy, fn)  ←─高层封装─→  thread +     │
│  (最简接口)                     packaged_task + future │
└─────────────────────────────────────────────────┘
```

| 组件 | 角色 | 何时用 |
|------|------|--------|
| `promise<T>` | 写入端 | 手动控制设值时机 |
| `future<T>` | 读取端（一次性） | 单消费者 |
| `shared_future<T>` | 读取端（多读者） | 多消费者 |
| `packaged_task<F>` | 包装函数 | 把现有函数变成异步任务 |
| `std::async` | 一站式 | 简单场景（但有坑，见 4.3） |

## 常见错误（新手踩坑）

### 错误 1：get() 调用两次

```cpp
std::future<int> f = p.get_future();
int v1 = f.get();  // OK
int v2 = f.get();  // UB！future 已失效
// 可能抛 std::future_error，也可能是 UB
```

**修复**：多读者用 `shared_future`。

### 错误 2：promise 析构前没设值

```cpp
void buggy() {
    std::promise<int> p;
    std::future<int> f = p.get_future();
    // p 析构时没设值 → future 抛 std::future_error(broken_promise)
}
// f.get() 抛 broken_promise 异常
```

**修复**：确保在 promise 析构前 `set_value` 或 `set_exception`。

### 错误 3：promise 不可拷贝

```cpp
std::promise<int> p;
std::promise<int> p2 = p;  // 编译错误！
std::promise<int> p3 = std::move(p);  // OK，只能移动
```

**注意**：promise 和 future 都不可拷贝，只能移动。要共享用 `shared_future`。

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 结果传递 | 手动 mutex + cv | promise/future |
| 异常传播 | 不可能（跨线程无异常） | 自动传播 |
| 多读者 | 手动广播 | `shared_future` |
| 样板代码 | 多 | 少 |
| 类型安全 | void* 转型 | 模板 |

## HFT 关联

- **避免在纳秒级热路径用**：promise/future 内部有共享状态 + atomic + 可能的堆分配，延迟不可控（~100ns-1μs）。热路径用 SPSC 队列 + 序列号更可控。
- **适合启动/关闭阶段**：HFT 启动时各组件初始化结果用 future 传递——简洁且异常安全。
- **超时等待**：`future::wait_for` 支持超时——等待交易所响应用超时避免无限阻塞。

## 代码自测

### Q1: 下列代码会怎样？

```cpp
std::promise<int> p;
auto f = p.get_future();
// 没有 set_value，p 析构
// ...
f.get();  // 会怎样？
```

<details>
<summary>答案与复习指引</summary>

**抛 `std::future_error(broken_promise)`**。promise 析构时如果没设值，对应的 future 会进入 broken 状态。`f.get()` 抛异常。

修复：确保 promise 析构前调用 `set_value` 或 `set_exception`。

复习：promise 和 future 是一对——promise 析构前必须设值，否则 future 报 broken_promise。
</details>

### Q2: 下列代码正确吗？

```cpp
std::future<int> f = std::async(std::launch::async, [] { return 42; });

std::thread t1([&f] { std::cout << f.get(); });
std::thread t2([&f] { std::cout << f.get(); });
t1.join(); t2.join();
```

<details>
<summary>答案与复习指引</summary>

**不正确**。`future::get()` 只能调一次——第二个线程调用 `get()` 是 UB（可能抛 `future_error`）。

修复：用 `shared_future`：
```cpp
auto sf = std::async(std::launch::async, [] { return 42; }).share();
std::thread t1([&sf] { std::cout << sf.get(); });
std::thread t2([&sf] { std::cout << sf.get(); });
```

复习：`future` 是单消费者的，`shared_future` 是多消费者的。
</details>

### Q3: 下列代码如何传播异常？

```cpp
std::promise<int> p;
auto f = p.get_future();

std::thread t([&p] {
    try {
        throw std::runtime_error("oops");
    } catch (...) {
        p.set_exception(std::current_exception());
    }
});
t.detach();

try {
    f.get();  // 会怎样？
} catch (const std::exception& e) {
    std::cout << e.what();  // 打印 "oops"
}
```

<details>
<summary>答案与复习指引</summary>

**异常正确传播**。`p.set_exception(std::current_exception())` 把异常存入共享状态。`f.get()` 检测到有异常，rethrow——消费者 catch 到 `runtime_error("oops")`。

这是 promise/future 相比 C 的共享变量的优势——跨线程异常传播。

复习：`set_exception` + `get` = 跨线程异常传播。生产者 catch，消费者 rethrow。
</details>

### Q4: 为什么 HFT 热路径不用 promise/future？

<details>
<summary>答案与复习指引</summary>

promise/future 的实现开销：
1. **共享状态堆分配**：promise/future 内部有一个 `std::shared_ptr` 指向共享状态——可能触发堆分配（~100ns-1μs）。
2. **atomic 同步**：`set_value` 和 `get` 内部有 atomic 操作保证可见性。
3. **异常机制开销**：如果传播异常，异常机制本身有开销。

HFT 热路径用 SPSC（单生产者单消费者）无锁队列：
- 预分配环形缓冲区（无堆分配）
- 序列号 + atomic（最小同步开销）
- 无异常（错误用返回码/序列号表示）

复习：promise/future 适合"方便但不需要极致性能"的场景。HFT 热路径用无锁队列替代。
</details>

---

## 参考与延伸

- 下一节：[4.3 async 启动策略](03-async-policy.md)
- 回到：[第 4 章](README.md)
