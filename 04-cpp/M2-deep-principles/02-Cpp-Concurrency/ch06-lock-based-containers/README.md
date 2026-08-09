# 第 6 章 设计基于锁的并发数据结构

**Designing Lock-Based Concurrent Data Structures**

## 本章讲什么

从"用锁保护单个容器"升级到"设计本身就是线程安全的并发数据结构"。本章用栈、队列、哈希表三个例子，讲细粒度锁、锁分段、读写锁的工程化设计，以及如何权衡锁粒度与并发度。

## 要点

### 线程安全栈：粗粒度锁入门

```cpp
template <typename T>
class threadsafe_stack {
    std::stack<T> data;
    mutable std::mutex m;
public:
    void push(const T& v) {
        std::lock_guard<std::mutex> lk(m);
        data.push(v);
    }
    std::shared_ptr<T> pop() {
        std::lock_guard<std::mutex> lk(m);
        if (data.empty()) throw std::runtime_error("empty");
        auto res = std::make_shared<T>(data.top());
        data.pop();
        return res;   // 返回 shared_ptr 避免拷贝异常
    }
};
```

设计要点：
- **整操作持锁**：`pop` 把 top+pop 合并在锁内，避免接口级竞争。
- **返回 `shared_ptr`** 而非 `T`：若 `T` 拷贝构造抛异常，数据已 pop 出来就丢了；`shared_ptr` 构造在锁内完成，pop 不抛异常。
- **空栈返回 `shared_ptr(nullptr)` 或抛异常**，不返回 `bool`+出参——让调用方选择。

### 线程安全队列：细粒度锁

单 mutex 的队列 push 和 pop 互斥。用**头尾各一把锁**可以并行入队出队：

```cpp
template <typename T>
class threadsafe_queue {
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex, tail_mutex;
    std::unique_ptr<node> head;
    node* tail;
    // ...
};
```

关键技巧：**dummy 头节点**。head 永远指向 dummy，tail 指向最后一个。push 只动 tail_mutex，pop 只动 head_mutex——只有当 head==tail（仅剩 dummy）时才需要同时锁两把。

### 锁分段哈希表

```cpp
template <typename K, typename V, size_t N = 16>
class concurrent_map {
    std::array<std::map<K,V>, N> buckets;
    std::array<std::mutex, N> locks;
    size_t idx(const K& k) const { return std::hash<K>{}(k) % N; }
public:
    void put(const K& k, const V& v) {
        size_t i = idx(k);
        std::lock_guard<std::mutex> lk(locks[i]);
        buckets[i][k] = v;
    }
};
```

N 个桶 N 把锁，不同桶的操作完全并行。N 越大并发度越高，但内存开销和 cache 压力也越大。Java 的 `ConcurrentHashMap` 就是这个思路（JDK 7 分段锁，JDK 8 改 CAS+单桶 synchronized）。

### 设计准则

| 准则 | 说明 |
|------|------|
| 真正的整操作 | 逻辑操作从头到尾持锁，不留窗口 |
| 锁粒度够细 | 不同部分用不同锁，提高并行度 |
| 避免死锁 | 锁序一致，用 `scoped_lock` 同时锁多把 |
| 异常安全 | 持锁期间的操作不能让结构处于半破坏态 |
| 考虑等待 | `try_pop` 非阻塞 vs `wait_and_pop` 阻塞 |

### 读写锁的应用

读多写少的数据结构用 `shared_mutex`：`shared_lock` 并发读，`unique_lock` 独占写。但要注意**写饥饿**——如果读者不断，写者可能长时间拿不到锁。

## HFT 关联

- **锁分段做行情字典**：合约→回调的映射表，读多写少（盘前写、盘中读）。用 16 分段锁，读用 `shared_lock`。
- **细粒度队列做策略分发**：多策略并行消费行情，用头尾分离锁的队列让入队出队不互斥。
- **避免热路径用 `shared_mutex`**：`shared_mutex` 的读路径有原子计数器开销，极高频率下不如无锁。热路径用单线程写 + 多线程无锁读（`atomic` 序列号 + 不可变快照）。
- **dummy 节点技巧**：SPSC 无锁队列也用 dummy 简化空/满判断，思路同此。
- **异常安全与订单状态**：改订单状态时若抛异常，结构不能处于半提交态——用 `shared_ptr` 传递确保要么完整要么不产生副作用。

## 自测题

1. 线程安全栈为什么 `pop` 要返回 `shared_ptr<T>` 而不是 `T`？
2. 线程安全队列如何用头尾两把锁实现 push/pop 并行？dummy 头节点的作用是什么？
3. 锁分段哈希表为什么能提高并发度？分段数如何选择？
4. 设计并发数据结构时，"整操作持锁"是什么意思？为什么不能拆成多个加锁小操作？
5. HFT 热路径为什么可能不用 `shared_mutex` 而用单写多读的无锁快照？

## 代码自测

### Q1: 粗粒度锁的瓶颈
```cpp
template<typename T>
class ThreadSafeQueue_Coarse {
    std::queue<T> q;
    std::mutex m;
public:
    void push(T v) { std::lock_guard<std::mutex> lk(m); q.push(std::move(v)); }
    bool pop(T& v) { std::lock_guard<std::mutex> lk(m); if(q.empty()) return false; v=q.front(); q.pop(); return true; }
};
```
> 这个队列在高并发下有什么性能问题？如何改进？

<details>
<summary>答案与复习指引</summary>

**粗粒度锁瓶颈**：一个 mutex 保护整个队列，所有 push/pop 操作互斥。高并发下线程排队等锁，吞吐量不随核数增长（串行化点）。

**改进方向**：
1. **细粒度锁**：分别锁 head 和 tail（`std::mutex head_m, tail_m`），push 只锁 tail，pop 只锁 head——允许同时 push 和 pop。
2. **无锁队列**：用 `atomic` + CAS 实现（见 ch07）。
3. **线程本地队列 + 批量迁移**：每个线程有本地队列（无锁），偶尔批量迁移（work stealing）。

**HFT**：订单队列用无锁或细粒度锁，避免热路径等锁。

**复习：** → [细粒度锁](./README.md)
</details>
