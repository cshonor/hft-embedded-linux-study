# Item 37：让 std::thread 在所有路径都不可联结（joinable）

> 第 7 章 · Item 37 · 上一节：[Item 36 启动策略](item36-launch-policy.md)

## 为什么要学这个（先建立直觉）

C 程序员用 `pthread` 时，忘了 `join` 只是资源泄漏——进程不会崩：

```c
pthread_t tid;
pthread_create(&tid, NULL, worker, NULL);
// 忘了 pthread_join → 线程资源泄漏，但进程不崩
// 或者提前 return → 线程可能还在跑
```

C++ 的 `std::thread` 更严格——**析构时如果仍 `joinable`（既未 `join` 也未 `detach`），直接 `std::terminate` 拉崩整个进程**：

```cpp
void f() {
    std::thread t(work);
    // 如果这里抛异常或 return ...
}  // t 析构时仍 joinable → std::terminate！进程崩溃！
```

这是 C++ 比 C 更严格的安全检查——强制你处理线程的生命周期。

---

## 这节讲什么

`std::thread` 析构时若仍 `joinable`（既未 `join` 也未 `detach`）→ **`std::terminate`**。用 RAII 保证所有路径安全。

---

## 核心问题

### 析构时 joinable = terminate

```cpp
void f() {
    std::thread t(work);
    // ... 如果这里抛异常或 return ...
}  // t 析构时仍 joinable → std::terminate！
```

### RAII 守卫

```cpp
class ThreadGuard {
    std::thread t;
public:
    explicit ThreadGuard(std::thread&& th) : t(std::move(th)) {}
    ~ThreadGuard() { if (t.joinable()) t.join(); }  // 析构时自动 join
    ThreadGuard(const ThreadGuard&) = delete;       // 禁止拷贝
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

void f() {
    ThreadGuard g(std::thread(work));
    // 即使抛异常，g 析构 → join → 安全
}
```

### detach 的风险

```cpp
void f() {
    std::thread t([]{
        // 引用了局部变量
        do_something(local_var);  // local_var 可能已销毁！
    });
    t.detach();  // 分离——线程在后台跑，但 f 返回后 local_var 销毁
}
// 比 join 更危险——detach 后线程的生命周期不受控
```

---

## 常见错误（新手踩坑）

**错误 1：异常路径忘了 join**
```cpp
void process() {
    std::thread t(work);
    if (error) throw std::runtime_error("error");  // 异常 → t 析构 → terminate
    t.join();
}
```
**修正：** 用 `ThreadGuard` 或 `try-catch + join`。

**错误 2：detach 后引用局部变量**
```cpp
void start() {
    int id = 42;
    std::thread t([&id]{ use(id); });  // 按引用捕获 id
    t.detach();  // start 返回后 id 销毁 → 线程访问悬垂引用 → UB
}
```
**修正：** 按值捕获或用 `shared_ptr`。

**错误 3：ThreadGuard 可拷贝导致多次 join**
```cpp
ThreadGuard g1(std::thread(work));
ThreadGuard g2 = g1;  // 如果允许拷贝 → 两个 guard 持有同一个 thread
// g1 析构 → join；g2 析构 → join 同一个 thread → UB
```
**修正：** `delete` 拷贝构造和拷贝赋值。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 线程创建 | `pthread_create` | `std::thread` | C++ 标准库 |
| 忘了 join | 资源泄漏 | `terminate` 进程崩溃 | C++ 更严格 |
| 异常安全 | 手动 `try-catch` | RAII 守卫 | 自动 |
| detach | 风险相同 | 风险相同 | 都需注意生命周期 |

**一句话总结：** C 程序员记住——`std::thread` 析构时如果仍 joinable 会 `terminate` 拉崩进程。用 RAII 守卫保证所有路径（含异常路径）都 `join` 或 `detach`。

---

## HFT 关联

- **守护进程崩溃**：HFT 守护进程里 `std::thread` 析构时若仍 joinable 会 `terminate` 拉崩整个进程——用 RAII 守卫或显式 `join`/`detach`。
- **异常安全**：HFT 进程在异常路径中必须保证线程安全 join——`ThreadGuard` 是标准做法。
- **热卸载**：策略热卸载时后台线程必须安全 join，不能 detach 后访问已销毁的策略对象。

---

## 自测题

1. `std::thread` 析构时仍 joinable 会发生什么？
2. 如何用 RAII 规避这个问题？
3. 为什么 `ThreadGuard` 要 `delete` 拷贝构造？
4. `detach` 后线程引用局部变量有什么风险？
5. 下面代码有什么问题？
```cpp
void work() {
    int data = 42;
    std::thread t([&data]{ process(data); });
    t.detach();
}
```

<details>
<summary>参考答案</summary>

1. 会调用 `std::terminate()`，整个程序直接终止。`std::thread` 的析构函数要求对象处于 non-joinable 状态（已 `join()` 或已 `detach()`、或被移动走、或从未关联线程）；若析构时 `joinable()` 为真，标准规定调用 `std::terminate`——不会因为异常栈展开而"顺手"帮你 join，也不会泄漏线程。这意味着任何一条提前 return / 抛异常的分支漏掉 join，程序就会崩。

2. 用一个 RAII 的线程包装类（笔记里的 `ThreadGuard`、C++20 的 `std::jthread`）：在它的析构函数里判断 `t.joinable()` 并调用 `t.join()`（析构里必须 join 而不能 detach，否则线程可能仍引用已销毁的对象）。这样无论函数是正常返回还是因异常栈展开，`thread` 对象的析构都会保证被 join，不会 `terminate`。C++20 起直接用 `std::jthread`：析构时自动 request_stop + join，并支持协作式取消。注意包装类作为成员/局部变量的声明顺序——它必须在被线程引用的对象**之后**销毁（或确保 join 先发生），否则 join 时线程可能已访问到销毁中的对象。

3. 因为 `ThreadGuard` 的语义是"独占地负责一个线程的 join"。如果允许拷贝，两个 guard 会持有同一个 `std::thread` 引用，析构时两次 join 同一个线程（第二次对已 non-joinable 的线程 join 是未定义行为/抛异常），或者其中一个被拷贝走后原对象仍在析构时 join。删除拷贝构造/拷贝赋值后，谁持有 guard 谁负责 join，责任唯一、语义清晰；需要转移所有权时可只提供移动构造（移动后源对象不再负责）。

4. `detach()` 之后线程独立运行，你再也无法 join 它、也无法知道它何时结束，编译器和标准都不会帮你延长它捕获的引用目标的生命周期。因此若线程按引用（`[&]`）捕获了局部变量、`this`、局部锁或局部容器，创建线程的函数一返回，这些东西就被销毁，而线程可能仍在访问它们——变成**悬垂引用**，读写已销毁对象是未定义行为（读到垃圾值、偶发崩溃，且极难复现）。安全做法：按值捕获（拷贝一份数据进线程）、用 `shared_ptr` 共享所有权、或改用 join/RAII 保证线程先于数据销毁。

5. 严重 bug：`t.detach()` 后 `work()` 立即返回，局部变量 `data` 随即销毁；而线程体用 `[&data]` 按引用捕获了它，线程可能还在运行并访问那块已失效的栈内存——未定义行为。修正方案（按推荐程度）：①按值捕获 `std::thread t([data]{ process(data); });`；②改用 RAII + join，让 `work()` 在线程结束后才返回（C++20 用 `std::jthread` 即可）；③若确实要 detach，则把数据共享为 `shared_ptr`（`[p = std::make_shared<int>(42)]{ process(*p); }`），让生命周期独立于创建者。另外 detach 的线程还会引用已销毁的进程资源，程序退出时应有明确的停止协议，不能靠"反正进程要退了"。

</details>

---

## 参考与延伸

- 下一节：[Item 38 句柄析构行为](item38-handle-destruction.md)
- 回到：[第 7 章](README.md)
