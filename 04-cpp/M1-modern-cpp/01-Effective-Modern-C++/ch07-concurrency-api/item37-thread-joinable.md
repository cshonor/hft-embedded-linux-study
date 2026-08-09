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

---

## 参考与延伸

- 下一节：[Item 38 句柄析构行为](item38-handle-destruction.md)
- 回到：[第 7 章](README.md)
