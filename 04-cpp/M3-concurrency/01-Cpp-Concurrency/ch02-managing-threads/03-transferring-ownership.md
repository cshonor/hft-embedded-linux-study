# 2.3 转移所有权

> 第 2 章 · 上一节：[2.2 传参](02-passing-args.md) · 下一节：[2.4 RAII 守卫](04-raii-guard.md)

## 这节讲什么

`std::thread` 不可拷贝但可移动——这让线程对象可以存入容器（`vector<std::thread>`），是构建线程池的基础。

## 为什么要学这个（先建立直觉）

C 的 `pthread_t` 是一个裸值（通常是整数），可以随意拷贝：

```c
// C：pthread_t 可以拷贝（但没意义——两个句柄管同一线程）
pthread_t t;
pthread_create(&t, NULL, func, NULL);
pthread_t t2 = t;  // 拷贝——t2 和 t 指向同一线程
// 问题：如果两个地方都调用 pthread_join(t, ...) 和 pthread_join(t2, ...)
// 行为未定义
```

C++ 的 `std::thread` 禁止拷贝（防止双重 join），但允许移动：

```cpp
// C++：std::thread 不可拷贝，可移动
std::thread t1(func);
// std::thread t2 = t1;  // 编译错误！不可拷贝
std::thread t2 = std::move(t1);  // OK：移动后 t1 不再 joinable
// t2 接管线程，t1 变成空状态
```

这个设计保证了**每个 joinable 的 thread 对象对应唯一一个线程**，不会出现两个对象管同一线程的混乱。

## 移动语义详解

### 基本移动

```cpp
std::thread t1(func);
std::thread t2 = std::move(t1);  // t1 → t2

// t1 此后不再 joinable()，析构不会 terminate
assert(!t1.joinable());  // true
assert(t2.joinable());   // true

t2.join();  // 用 t2 来 join
```

### 存入容器

```cpp
// vector<std::thread> 是线程池的基础
std::vector<std::thread> pool;

// 方式 1：push_back 临时对象（隐式移动）
pool.push_back(std::thread(worker, 1));  // 临时对象被移动进 vector

// 方式 2：emplace_back 直接构造
pool.emplace_back(worker, 2);

// 方式 3：先创建再移动
std::thread t(worker, 3);
pool.push_back(std::move(t));

// 统一 join
for (auto& t : pool)
    if (t.joinable()) t.join();
```

### 从函数返回 thread

```cpp
// 函数返回 std::thread（隐式移动）
std::thread make_worker(int id) {
    return std::thread([id] { std::cout << "worker " << id; });
    // 返回时移动（RVO 也可能直接构造）
}

auto t = make_worker(42);
t.join();
```

### 转移给 RAII 守卫

```cpp
class joining_thread {
    std::thread t;
public:
    explicit joining_thread(std::thread&& th) : t(std::move(th)) {}
    ~joining_thread() { if (t.joinable()) t.join(); }
};

joining_thread guard(std::thread(func));  // 移动进守卫
// 守卫析构时自动 join
```

| 操作 | 语法 | 效果 |
|------|------|------|
| 移动构造 | `t2 = std::move(t1)` | t1 变空，t2 接管 |
| 存入 vector | `pool.push_back(std::move(t))` | vector 持有线程 |
| 函数返回 | `return std::thread(...)` | RVO 或隐式移动 |
| 转移给守卫 | `guard(std::thread(func))` | 守卫管理生命周期 |

## 常见错误（新手踩坑）

### 错误 1：移动后对原对象调用 join

```cpp
std::thread t1(func);
std::thread t2 = std::move(t1);
t1.join();  // 错误！t1 不再 joinable
// 在非 joinable 对象上调用 join 是 UB
```

**修复**：移动后只对新对象操作，或加 `if (t1.joinable()) t1.join();`。

### 错误 2：vector 扩容导致 thread 析构

```cpp
std::vector<std::thread> pool;
for (int i = 0; i < 100; i++) {
    pool.push_back(std::thread(worker, i));
    // vector 扩容时移动旧元素到新内存
    // 但如果移动构造抛异常，元素会被拷贝——而 thread 不可拷贝！
}
// 实际上 std::thread 的移动构造是 noexcept，所以安全
// 但其他不可拷贝类型要注意
```

**注意**：`std::thread` 的移动构造函数是 `noexcept` 的，所以 `vector` 扩容时不会出问题。但这是 `std::thread` 的特殊保证，不是通用的。

### 错误 3：忘记 vector 中的 thread 需要在 vector 析构前 join

```cpp
{
    std::vector<std::thread> pool;
    pool.emplace_back(worker, 1);
    pool.emplace_back(worker, 2);
    // vector 析构时，thread 对象析构
    // 如果仍 joinable → terminate！
}
```

**修复**：在 vector 析构前循环 join。

```cpp
for (auto& t : pool)
    if (t.joinable()) t.join();
```

## 和 C 的区别

| 特性 | C (pthread_t) | C++ (std::thread) |
|------|---------------|-------------------|
| 可拷贝 | 是（但无意义/危险） | 否（编译期禁止） |
| 可移动 | 不适用（是值类型） | 是（`std::move`） |
| 存入容器 | 手动管理数组 | `vector<std::thread>` |
| 所有权 | 模糊（谁都管） | 明确（唯一 joinable 对象） |
| 移动构造 noexcept | N/A | 是（保证 vector 安全） |

## HFT 关联

- **线程池 = `vector<std::thread>` + 任务队列**：HFT 启动时创建固定数量线程存入 vector，绑核，循环从无锁队列取任务。
- **线程转移用于负载均衡**：多 NUMA 节点场景，可以把线程从一个核心迁移到另一个（通过移动 thread 对象 + 重新绑核）。
- **vector 预分配**：HFT 线程池在启动时 `reserve` 固定大小，避免运行时扩容。

## 代码自测

### Q1: 下列代码能编译吗？

```cpp
std::thread t1(func);
std::thread t2 = t1;  // 拷贝构造
```

<details>
<summary>答案与复习指引</summary>

**编译错误**。`std::thread` 的拷贝构造函数被 `delete` 了——不可拷贝。

修复：`std::thread t2 = std::move(t1);`

复习：`std::thread` 不可拷贝（防止两个对象管同一线程导致双重 join），但可移动。
</details>

### Q2: 下列代码安全吗？

```cpp
std::vector<std::thread> pool;
for (int i = 0; i < 4; i++)
    pool.emplace_back(worker, i);
// 没有 join
// vector 析构
```

<details>
<summary>答案与复习指引</summary>

**不安全**。vector 析构时调用每个 `std::thread` 的析构函数。如果线程仍 joinable，析构调用 `std::terminate` 拉崩进程。

修复：vector 析构前循环 join：
```cpp
for (auto& t : pool)
    if (t.joinable()) t.join();
```

复习：`std::thread` 对象的生命周期管理是 C++ 多线程编程的第一要务。
</details>

### Q3: 下列代码的输出是什么？

```cpp
std::thread make_thread() {
    return std::thread([] { std::cout << "running"; });
}
auto t = make_thread();
std::cout << "created";
t.join();
```

<details>
<summary>答案与复习指引</summary>

输出可能是 `createdrunning` 或 `runningcreated`，取决于线程调度。

`make_thread` 返回 `std::thread` 时，编译器可能用 RVO（直接在调用者的栈上构造）或移动语义。无论哪种，线程在函数返回前就开始运行。

复习：函数返回 `std::thread` 是安全的——RVO 或隐式移动保证不会 terminate。
</details>

### Q4: 为什么 `std::thread` 的移动构造函数必须是 `noexcept`？

<details>
<summary>答案与复习指引</summary>

因为 `std::vector` 扩容时需要移动元素。如果移动构造函数可能抛异常：
1. vector 扩容时移动部分元素后抛异常
2. 剩余元素还在旧内存中
3. 旧内存已被释放 → 元素丢失

`std::thread` 的移动构造是 `noexcept` 的，所以 `vector<std::thread>` 扩容时要么全部移动成功，要么不移动——不会出现半移动状态。

复习：`noexcept` 移动构造是"可存入标准容器"的关键保证。`std::thread`、`std::unique_ptr` 等都满足。
</details>

---

## 参考与延伸

- 下一节：[2.4 RAII 守卫](04-raii-guard.md)
- 回到：[第 2 章](README.md)
