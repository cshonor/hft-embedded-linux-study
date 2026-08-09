# 3.5 读写锁 std::shared_mutex（C++17）

> 第 3 章 · 上一节：[3.4 初始化保护](04-init-protection.md) · 下一章：[第 4 章 同步操作](../ch04-synchronizing-operations/README.md)

## 这节讲什么

多读少写场景用 `shared_mutex`：读用 `shared_lock`（多读者并发），写用 `unique_lock`（独占）。适合读多写少的数据保护。

## 为什么要学这个（先建立直觉）

C 程序员用 `pthread_rwlock_t` 做读写锁：

```c
// C：pthread 读写锁
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

// 读者
pthread_rwlock_rdlock(&rwlock);   // 共享锁
read_data();
pthread_rwlock_unlock(&rwlock);

// 写者
pthread_rwlock_wrlock(&rwlock);   // 独占锁
write_data();
pthread_rwlock_unlock(&rwlock);
```

C++17 标准化了读写锁：

```cpp
// C++17：shared_mutex
std::shared_mutex rwlock;

// 读者：多个线程可同时持有共享锁
{
    std::shared_lock<std::shared_mutex> lk(rwlock);  // 共享锁
    read_data();  // 多读者并发
}

// 写者：独占
{
    std::unique_lock<std::shared_mutex> lk(rwlock);  // 独占锁
    write_data();  // 写者独占
}
```

为什么需要读写锁？普通 `mutex` 即使是读操作也互斥——但读操作不修改数据，多个读者可以安全并发。

## 核心用法详解

### 基本 read/write

```cpp
#include <shared_mutex>

std::shared_mutex mtx;
int shared_data = 0;

// 读者
int read_data() {
    std::shared_lock<std::shared_mutex> lk(mtx);  // 共享锁
    return shared_data;
}

// 写者
void write_data(int val) {
    std::unique_lock<std::shared_mutex> lk(mtx);  // 独占锁
    shared_data = val;
}
```

### 升级/降级

```cpp
// 从读锁升级到写锁——标准库不直接支持，需要先释放再获取
void upgrade() {
    std::shared_lock<std::shared_mutex> rlk(mtx);  // 读锁
    auto data = read();
    rlk.unlock();  // 释放读锁

    std::unique_lock<std::shared_mutex> wlk(mtx);  // 写锁
    // 注意：其他线程可能在此间隙修改数据！
    write(data + 1);
}

// C++14 的 shared_lock 不支持原子升级
// 如果需要原子升级，用 boost::upgrade_lock 或自己封装
```

### 性能特征

```
读者数量 vs 性能：

普通 mutex：     1 读者 → 1 线程并发（其余等待）
shared_mutex：   N 读者 → N 线程并发（读不互斥）
                 1 写者 → 独占（所有读者和写者等待）

何时 shared_mutex 更快：
  读:写 = 100:1 → shared_mutex 快很多
  读:写 = 1:1   → shared_mutex 更慢（管理共享锁有开销）
  读:写 = 1:100 → 普通 mutex 更好
```

## 常见错误（新手踩坑）

### 错误 1：读多写少才用，别滥用

```cpp
// 错误：读写比接近 1:1 时用 shared_mutex 反而更慢
std::shared_mutex mtx;
void worker() {
    // 每次调用都一半概率写——shared_mutex 的锁管理开销 > 并行收益
    if (rand() % 2) {
        std::unique_lock<std::shared_mutex> lk(mtx);
        data++;
    } else {
        std::shared_lock<std::shared_mutex> lk(mtx);
        use(data);
    }
}
```

**修复**：读写比 < 10:1 时用普通 `mutex`。压测决定。

### 错误 2：写者饥饿

```cpp
// 某些实现中，持续有读者时写者永远等不到锁
std::shared_mutex mtx;
// 100 个读者持续循环读
for (int i = 0; i < 100; i++)
    threads.emplace_back([&] {
        while (running) {
            std::shared_lock<std::shared_mutex> lk(mtx);
            read();
        }
    });
// 写者可能永远等不到独占锁——所有读者释放后立即有新读者获取
```

**注意**：C++17 标准不保证写者优先。如果写者饥饿是问题，需要自定义读写锁或用 `std::condition_variable` 实现公平调度。

### 错误 3：shared_lock 不是"轻量"的

```cpp
// 错误：以为 shared_lock 没有开销
// shared_mutex 内部维护读者计数（atomic），每次获取/释放都有原子操作
std::shared_lock<std::shared_mutex> lk(mtx);  // 仍有 atomic RMW 开销
```

**注意**：`shared_mutex` 的共享锁获取/释放涉及原子计数器操作（~10-50ns），比普通 `mutex` 的 `lock` 略慢（但允许多读者并发）。单读者时 `shared_mutex` 比 `mutex` 慢。

## 和 C 的区别

| 特性 | C (pthread) | C++ (std) |
|------|-------------|-----------|
| 读写锁 | `pthread_rwlock_t` | `std::shared_mutex`（C++17） |
| 共享锁 | `pthread_rwlock_rdlock` | `std::shared_lock` |
| 独占锁 | `pthread_rwlock_wrlock` | `std::unique_lock` |
| RAII | 无（手动） | 有 |
| 写者优先 | 实现相关 | 不保证 |

## HFT 关联

- **行情快照**：多策略读、单线程写用 `shared_mutex`，读不互斥——策略并行读快照不互相阻塞。
- **配置缓存**：配置在启动时写一次，运行时多线程读——`shared_mutex` 让多线程并行读配置。
- **热路径仍避锁**：即使 `shared_mutex` 的读锁开销（~10-50ns atomic）也比无锁访问慢。HFT 热路径用 `seq_cst` 原子序列号或 RCPU（Read-Copy-Update）模式替代读写锁。

## 代码自测

### Q1: 下列代码有什么问题？

```cpp
std::shared_mutex mtx;
std::vector<int> data;

// 线程 1（读者）
{
    std::shared_lock<std::shared_mutex> lk(mtx);
    for (auto v : data) process(v);  // 遍历
}

// 线程 2（写者）
{
    std::unique_lock<std::shared_mutex> lk(mtx);
    data.clear();  // 清空
}
```

<details>
<summary>答案与复习指引</summary>

**没有功能问题**——读写锁正确保护了数据。但如果线程 1 正在遍历时线程 2 等待，遍历时间长则写者延迟大。

如果 `process(v)` 很慢，考虑：先在锁内拷贝数据引用，解锁后再处理。或用 RCU 模式（写者拷贝一份修改，原子替换指针，读者用旧版本完成后释放）。

复习：读写锁适合"读快写少"的场景。如果读操作耗时长，写者延迟也会大。
</details>

### Q2: 下列场景适合用 shared_mutex 吗？

```cpp
// 场景：每秒 1000 次读，1 次写
std::shared_mutex mtx;
Config config;

Config read() {
    std::shared_lock<std::shared_mutex> lk(mtx);
    return config;
}
void update(Config c) {
    std::unique_lock<std::shared_mutex> lk(mtx);
    config = c;
}
```

<details>
<summary>答案与复习指引</summary>

**适合**。读写比 1000:1 是典型的"读多写少"场景，`shared_mutex` 让 1000 次读操作并行（互不阻塞），只有写操作时才独占。

但要注意 `read()` 返回 `Config` 的拷贝——如果 Config 很大，拷贝开销可能超过锁开销。考虑返回 `shared_ptr<const Config>` 避免拷贝。

复习：读写比 > 10:1 是 `shared_mutex` 的甜区。
</details>

### Q3: 下列代码安全吗？

```cpp
std::shared_mutex mtx;

void reader() {
    std::shared_lock lk(mtx);
    read();
    lk.unlock();
    // 不重新加锁，直接访问数据
    read();  // 无锁访问！
}
```

<details>
<summary>答案与复习指引</summary>

**不安全**。`unlock()` 后不再有锁保护，第二个 `read()` 可能与写者并发——数据竞争。

修复：把两个 `read()` 都放在锁内，或使用 RCU 模式。

复习：锁的作用域必须覆盖所有数据访问——解锁后就不能再访问受保护的数据。
</details>

### Q4: 为什么 HFT 热路径用序列号替代 shared_mutex？

<details>
<summary>答案与复习指引</summary>

`shared_mutex` 的共享锁仍有原子操作开销（~10-50ns），在高频热路径上累积可观。

序列号方案（无锁读）：
```cpp
std::atomic<uint64_t> seq{0};
Data data;

// 写者
void write(Data d) {
    data = d;
    seq.store(seq.load() + 1, std::memory_order_release);  // 版本+1
}

// 读者（无锁）
Data read() {
    uint64_t s1, s2;
    Data d;
    do {
        s1 = seq.load(std::memory_order_acquire);
        d = data;  // 读数据
        s2 = seq.load(std::memory_order_acquire);
    } while (s1 != s2);  // 如果版本变了，重读
    return d;  // 保证读到一致快照
}
```

优势：读者完全无锁（只有 atomic load），延迟可预测。劣势：写者频繁更新时读者可能重试多次。

复习：序列号模式 = 无锁版的读写锁。HFT 用它避免 `shared_mutex` 的原子 RMW 开销。
</details>

---

## 参考与延伸

- 下一章：[第 4 章 同步操作](../ch04-synchronizing-operations/README.md)
- 回到：[第 3 章](README.md)
