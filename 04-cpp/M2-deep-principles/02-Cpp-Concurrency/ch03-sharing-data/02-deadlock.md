# 3.2 死锁及避免

> 第 3 章 · 上一节：[3.1 mutex + RAII 锁](01-mutex-raii.md) · 下一节：[3.3 接口级竞争](03-interface-race.md)

## 这节讲什么

死锁的四个必要条件与避免手段——`scoped_lock` 原子锁多资源、固定锁序、层级锁。理解死锁才能在设计时主动避免。

## 为什么要学这个（先建立直觉）

C 程序员可能已经遇到过死锁：

```c
// C：经典死锁——交叉锁序
pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void* thread_a(void* arg) {
    pthread_mutex_lock(&m1);
    // ... 此处被调度切换 ...
    pthread_mutex_lock(&m2);  // 等 m2（被 thread_b 持有）
    // 永远等不到 → 死锁
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    return NULL;
}

void* thread_b(void* arg) {
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m1);  // 等 m1（被 thread_a 持有）→ 死锁！
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
    return NULL;
}
```

C++17 提供了 `std::scoped_lock` 原子地锁多个 mutex，从语言层面消灭这种死锁：

```cpp
// C++17：scoped_lock 原子锁多个——不会死锁
void thread_a() {
    std::scoped_lock lk(m1, m2);  // 原子获取 m1 和 m2
    // ... 安全操作 ...
}
void thread_b() {
    std::scoped_lock lk(m2, m1);  // 顺序不同也没关系！
    // ... 安全操作 ...
}
```

## 死锁四条件

死锁发生需要**四个条件同时满足**（Coffman 条件），打破任一即可避免：

| 条件 | 含义 | 打破方式 |
|------|------|----------|
| 互斥 | 资源同一时刻只能被一个线程占有 | 有些资源天生互斥（如写锁） |
| 占有等待 | 持有资源的同时等待另一个资源 | 一次性获取所有资源（`scoped_lock`） |
| 不可剥夺 | 不能强制剥夺线程持有的资源 | 超时锁（`try_lock_for`） |
| 循环等待 | 线程间形成环形等待链 | 固定锁序（全局规定加锁顺序） |

## 避免手段详解

### 手段 1：std::scoped_lock（C++17，首选）

```cpp
// 原子地同时锁多个 mutex——不会死锁
std::mutex m1, m2;

void safe_op() {
    std::scoped_lock lk(m1, m2);  // 内部用 "try and back-off" 算法
    // 操作两个受保护的数据...
}  // 析构同时释放两个锁

// 任意顺序都安全：
std::scoped_lock lk2(m2, m1);  // 也安全！
```

`scoped_lock` 内部实现类似 `std::lock(m1, m2)`——使用 try_lock 循环避免死锁：

```cpp
// std::lock 的简化逻辑
while (true) {
    m1.lock();
    if (m2.try_lock()) return;  // 成功获取两个
    m1.unlock();  // 失败，释放 m1，重试
    // 可能加微小延迟避免活锁
}
```

### 手段 2：固定锁序

```cpp
// 全局规定：永远先锁 m1 再锁 m2
// 所有线程遵守同一顺序 → 不会循环等待
void thread_a() {
    std::lock_guard<std::mutex> lk1(m1);  // 先 m1
    std::lock_guard<std::mutex> lk2(m2);  // 再 m2
}
void thread_b() {
    std::lock_guard<std::mutex> lk1(m1);  // 也先 m1！
    std::lock_guard<std::mutex> lk2(m2);  // 再 m2
}
// 不会死锁——两个线程都以相同顺序获取锁
```

### 手段 3：层级锁

```cpp
// 自定义层级锁——运行时检测锁序违反
class hierarchical_mutex {
    std::mutex m;
    const unsigned long level;
    static thread_local unsigned long current_level;
public:
    explicit hierarchical_mutex(unsigned long l) : level(l) {}

    void lock() {
        if (level >= current_level)  // 逆序！
            throw std::logic_error("mutex hierarchy violated");
        m.lock();
        unsigned long old = current_level;
        current_level = level;
        // 保存 old 以便 unlock 恢复
    }
    // ...
};

hierarchical_mutex high(10000), mid(5000), low(100);
// 规则：只能从高到低加锁
void safe() {
    std::lock_guard<hierarchical_mutex> h(high);  // level 10000
    std::lock_guard<hierarchical_mutex> m(mid);    // level 5000 < 10000 ✓
    std::lock_guard<hierarchical_mutex> l(low);     // level 100 < 5000 ✓
}
void buggy() {
    std::lock_guard<hierarchical_mutex> l(low);    // level 100
    std::lock_guard<hierarchical_mutex> m(mid);    // 5000 > 100 → throw!
}
```

## 常见错误（新手踩坑）

### 错误 1：分别锁多个资源

```cpp
// 错误：分别锁 → 死锁窗口
std::lock_guard<std::mutex> lk1(m1);
std::lock_guard<std::mutex> lk2(m2);  // 如果此时其他线程已锁 m2 且等 m1 → 死锁
```

**修复**：用 `std::scoped_lock lk(m1, m2);` 原子锁。

### 错误 2：锁序不一致

```cpp
// 错误：不同函数以不同顺序获取相同锁
void transfer(A& a, B& b) {
    std::lock_guard<std::mutex> lka(a.mtx);
    std::lock_guard<std::mutex> lkb(b.mtx);
    // ...
}
void transfer_back(B& b, A& a) {
    std::lock_guard<std::mutex> lkb(b.mtx);  // 先 b
    std::lock_guard<std::mutex> lka(a.mtx);  // 后 a → 与 transfer 相反 → 死锁！
}
```

**修复**：全局规定锁序（如按内存地址排序），或用 `scoped_lock`。

### 错误 3：在持锁状态下调用外部代码

```cpp
// 错误：持锁调用 callback——callback 可能也加锁
std::lock_guard<std::mutex> lk(m);
callback();  // callback 内部可能 lock(m) → 死锁（非递归 mutex）
// 或 callback lock 了其他锁 → 可能死锁
```

**修复**：在锁外调用 callback，或用递归锁（不推荐），或限制 callback 的锁行为。

## 和 C 的区别

| 特性 | C (pthread) | C++ (std) |
|------|-------------|-----------|
| 原子锁多个 | 手写 try-lock 循环 | `std::scoped_lock`（C++17） |
| 超时锁 | `pthread_mutex_timedlock` | `try_lock_for`（`timed_mutex`） |
| 锁序检测 | 手动 | 手动（层级锁模式） |
| 死锁检测 | 无 | 无（但 TSan 可检测） |

## HFT 关联

- **`scoped_lock` 同时锁多资源**：订单簿跨多结构操作用 `scoped_lock` 原子锁，避免分别锁的死锁窗口。
- **避免持锁做 IO**：HFT 在持锁状态下绝不做网络 IO——IO 阻塞导致锁持有时间不可预测，放大死锁概率。
- **ThreadSanitizer 检测死锁**：HFT 测试环境用 TSan 运行，能检测潜在死锁（即使没触发）。

## 代码自测

### Q1: 下列代码会死锁吗？

```cpp
std::mutex m1, m2;

void thread_a() {
    std::lock_guard<std::mutex> lk1(m1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::lock_guard<std::mutex> lk2(m2);
}

void thread_b() {
    std::lock_guard<std::mutex> lk1(m2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::lock_guard<std::mutex> lk2(m1);
}
```

<details>
<summary>答案与复习指引</summary>

**会死锁**。thread_a 先锁 m1 等 m2，thread_b 先锁 m2 等 m1——循环等待。

修复：用 `std::scoped_lock lk(m1, m2);` 替代分别加锁。

复习：分别锁多个资源 + 顺序不一致 = 死锁。用 `scoped_lock` 原子锁避免。
</details>

### Q2: 下列代码安全吗？

```cpp
std::mutex m1, m2;

void func() {
    std::scoped_lock lk(m1, m2);
    // ...
}

void func_reversed() {
    std::scoped_lock lk(m2, m1);  // 顺序相反
    // ...
}
```

<details>
<summary>答案与复习指引</summary>

**安全**。`scoped_lock` 内部使用 try-lock 算法原子地获取所有锁，不依赖参数顺序。两个函数以不同顺序调用也不会死锁。

复习：`scoped_lock` 的核心优势——不关心锁的顺序，内部保证不死锁。
</details>

### Q3: 下列层级锁代码会怎样？

```cpp
hierarchical_mutex high(10000), mid(5000), low(100);

void func() {
    std::lock_guard<hierarchical_mutex> l(low);     // level 100
    std::lock_guard<hierarchical_mutex> h(high);    // level 10000 > 100
}
```

<details>
<summary>答案与复习指引</summary>

**抛异常**（`mutex hierarchy violated`）。层级锁规则是"只能从高到低加锁"——先锁 low(100) 再锁 high(10000) 是逆序。

修复：调换顺序——先 high 再 mid 再 low。

复习：层级锁在运行时检测锁序违反——帮助开发者发现潜在的死锁路径。
</details>

### Q4: 为什么 HFT 在持锁状态下不做网络 IO？

<details>
<summary>答案与复习指引</summary>

1. **持锁时间不可预测**：网络 IO 可能阻塞几毫秒到几秒，期间锁被持有，其他线程全部阻塞。
2. **放大死锁概率**：持锁时间越长，其他线程等待该锁的时间越长，与其他锁形成循环等待的概率越大。
3. **延迟抖动**：HFT 要求锁持有时间 < 微秒级，网络 IO 引入毫秒级抖动不可接受。

HFT 原则：锁内只做内存操作（读/写共享数据），IO 在锁外完成。

复习：锁的黄金法则——最小化临界区，锁内不做 IO/计算/回调。
</details>

---

## 参考与延伸

- 下一节：[3.3 接口级竞争](03-interface-race.md)
- 回到：[第 3 章](README.md)
