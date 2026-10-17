# 第 3 章 共享数据

**Sharing Data Between Threads**

## 本章讲什么

多线程共享可变数据的核心难题——数据竞争与同步。本章讲 `mutex` 的正确使用、`lock_guard`/`unique_lock` 的 RAII、死锁的成因与避免、以及细粒度保护共享数据的设计。

## 要点

### `mutex` + RAII 锁

```cpp
std::mutex m;
std::lock_guard<std::mutex> lk(m);   // 构造加锁，析构解锁（RAII）
```
`lock_guard` 是最简 RAII 锁——构造即锁、析构即解。**永远不要手写 `lock()`/`unlock()`**（异常路径会忘记解锁）。

`unique_lock` 更灵活——可延迟加锁（`defer_lock`）、可提前解锁（`unlock()`）、可转移所有权。代价略大于 `lock_guard`。

### 死锁及避免

死锁四条件：互斥、占有等待、不可剥夺、循环等待。避免手段：

| 手段 | 说明 |
|------|------|
| `std::lock(m1, m2)` | 原子地同时锁多个（避免分别锁导致死锁） |
| `std::scoped_lock`（C++17） | RAII 版的 `std::lock`，推荐 |
| 固定锁序 | 全局规定加锁顺序，不逆序 |
| 层级锁 | 用 `std::lock_guard` + 自定义层级校验 |

### 接口级竞争

保护要覆盖**整个逻辑操作**，而非单步：
```cpp
// 错误：get 和 pop 各自加锁，中间窗口被其他线程插入
if (!stack.empty()) { auto v = stack.top(); stack.pop(); }
// 正确：用一个原子操作或持锁覆盖整个检查+取
```

### 初始化保护

```cpp
std::once_flag flag;
std::call_once(flag, []{ init(); });   // 只初始化一次，线程安全

static Config& inst() { static Config c; return c; }  // C++11 起线程安全
```
C++11 起 `static` 局部变量的初始化由编译器保证线程安全（`call_once` 语义）。

### 读写锁 `std::shared_mutex`（C++17）

多读少写场景用 `shared_mutex`：读用 `shared_lock`（多读者并发），写用 `unique_lock`（独占）。

## HFT 关联

- **热路径避锁**：mutex 有上下文切换 + 调度抖动风险，HFT 热路径用无锁结构（`atomic`/SPSC 队列）替代 mutex。
- **`scoped_lock` 同时锁多资源**：订单簿跨多结构操作用 `scoped_lock` 原子锁，避免分别锁的死锁窗口。
- **`shared_mutex` 读多写少**：行情快照（多策略读、单线程写）用 `shared_mutex`，读不互斥。
- **锁粒度**：HFT 锁要覆盖完整操作但尽量短，减少持锁时间。

## 自测题

1. 为什么永远不要手写 `lock()`/`unlock()`？`lock_guard` 如何保证异常安全？
2. 死锁四条件是什么？`std::scoped_lock` 如何避免分别锁导致的死锁？
3. "接口级竞争"是什么？为什么 `if(!empty()) top(); pop();` 是错的？
4. C++11 起 `static` 局部变量的线程安全性由谁保证？
5. HFT 热路径为什么避免 mutex？用什么替代？

## 代码自测

### Q1: 死锁
```cpp
std::mutex m1, m2;

void threadA() {
    std::lock_guard<std::mutex> lk1(m1);
    std::lock_guard<std::mutex> lk2(m2);
    // do work
}
void threadB() {
    std::lock_guard<std::mutex> lk1(m2);
    std::lock_guard<std::mutex> lk2(m1);
    // do work
}
```
> 同时启动 threadA 和 threadB 会发生什么？怎么避免？

<details>
<summary>答案与复习指引</summary>

**死锁**：threadA 持 m1 等 m2，threadB 持 m2 等 m1，永久阻塞。

**避免方法**：
1. **`std::lock(m1, m2)`**：C++11 提供的原子多锁获取（内部用避免死锁算法），配合 `std::adopt_lock`：
```cpp
std::lock(m1, m2);
std::lock_guard<std::mutex> lk1(m1, std::adopt_lock);
std::lock_guard<std::mutex> lk2(m2, std::adopt_lock);
```
2. **C++17 `std::scoped_lock`**：一行搞定 `std::scoped_lock lk(m1, m2);`
3. **固定锁顺序**：所有线程按相同顺序加锁。

**复习：** → [死锁](./README.md)
</details>

### Q2: 接口层面的竞态
```cpp
std::stack<int> s;
void consumer() {
    if (!s.empty()) {      // 检查
        int val = s.top(); // 取值
        s.pop();           // 弹出
        process(val);
    }
}
```
> 多个 consumer 线程同时运行时这段代码有什么问题？

<details>
<summary>答案与复习指引</summary>

**TOCTOU 竞态**（Time-of-Check to Time-of-Use）：`empty()` 检查和 `top()`/`pop()` 之间可能被其他线程插入。

场景：两个 consumer 都看到 `!empty()`，都调 `top()`（拿到同一个值），都调 `pop()`——一个元素被处理两次，另一个元素被跳过。

**根因**：`std::stack` 的接口设计为非线程安全——`empty()`、`top()`、`pop()` 是分离操作。标准容器不保证并发安全。

**对策**：用外部 mutex 保护整个操作序列，或用线程安全队列（`pop()` 返回值，一步完成）。

**复习：** → [接口层面的竞态](./README.md)
</details>
