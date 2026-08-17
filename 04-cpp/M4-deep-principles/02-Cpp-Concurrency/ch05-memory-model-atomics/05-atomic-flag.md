# 5.5 原子标志的最简同步

> 第 5 章 · 上一节：[5.4 CAS](04-cas.md) · 下一节：[5.6 volatile ≠ atomic](06-volatile-not-atomic.md)

## 这节讲什么

`atomic<bool>` + release/acquire 是最简单的跨线程同步模式——一个线程写数据+置标志，另一个线程读标志+读数据。这是无锁同步的基础模式。

## 为什么要学这个（先建立直觉）

C 程序员用 mutex 做线程间数据传递：

```c
// C：mutex 保护数据传递
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
int data = 0;
int ready = 0;

// 线程 A（生产者）
pthread_mutex_lock(&mtx);
data = 42;
ready = 1;
pthread_mutex_unlock(&mtx);

// 线程 B（消费者）
pthread_mutex_lock(&mtx);
while (!ready)
    pthread_cond_wait(&cv, &mtx);  // 等待
int val = data;
pthread_mutex_unlock(&mtx);
// 大量样板代码
```

如果只需要"一次性传递数据"，`atomic<bool>` + release/acquire 就够了——不需要 mutex：

```cpp
// C++：atomic<bool> 最简同步
std::atomic<bool> ready{false};
int data = 0;

// 线程 A（生产者）
data = 42;
ready.store(true, std::memory_order_release);  // release：data=42 对读者可见

// 线程 B（消费者）
while (!ready.load(std::memory_order_acquire)) {}  // acquire：看到 ready=true 后
std::cout << data;  // 一定读到 42
```

## 核心模式详解

### 发布-等待模式

```cpp
// 发布方（生产者）
void publish() {
    // 1. 准备数据（非原子操作）
    shared_data = compute();
    shared_metadata = generate();

    // 2. 发布标志（release：之前的所有写对读者可见）
    ready.store(true, std::memory_order_release);
}

// 等待方（消费者）
void consume() {
    // 1. 等待标志（acquire：之后的读看到 release 前的写）
    while (!ready.load(std::memory_order_acquire)) {
        // 可以加 _mm_pause() 或 yield() 减少总线争用
        std::this_thread::yield();
    }

    // 2. 安全读取数据（保证看到所有发布前的写）
    use(shared_data);    // 一定看到 compute() 的结果
    use(shared_metadata); // 一定看到 generate() 的结果
}
```

### 为什么不需要 mutex？

```
mutex 方式：
  生产者：lock → 写数据 → unlock（release）→ 消费者：lock（acquire）→ 读数据 → unlock
  mutex 提供：互斥 + 内存屏障

atomic 方式：
  生产者：写数据 → store(release) → 消费者：load(acquire) → 读数据
  atomic 提供：内存屏障（不需要互斥——只有一个写者一个读者）

区别：
  mutex 适合多写者多读者的临界区保护
  atomic<bool> 适合"一次性发布"的简单同步
```

### 变体：多重发布

```cpp
// 变体：循环发布（每次发布新数据）
std::atomic<uint32_t> seq{0};  // 序列号代替 bool
Data data;

void publish(Data d) {
    data = d;
    seq.store(seq.load(std::memory_order_relaxed) + 1,
              std::memory_order_release);  // 序列号 +1
}

void consume() {
    uint32_t s1, s2;
    do {
        s1 = seq.load(std::memory_order_acquire);  // 读序列号
        Data d = data;  // 读数据
        s2 = seq.load(std::memory_order_acquire);  // 再读序列号
    } while (s1 != s2);  // 如果序列号变了，重读（数据被修改了）
    // s1 == s2：读到的是一致的数据快照
    use(d);
}
```

## 常见错误（新手踩坑）

### 错误 1：用 relaxed 内存序

```cpp
// 错误：relaxed 不建立 happens-before
std::atomic<bool> ready{false};
int data = 0;

data = 42;
ready.store(true, std::memory_order_relaxed);  // relaxed！

while (!ready.load(std::memory_order_relaxed)) {}
std::cout << data;  // 可能不是 42！
```

**修复**：`release`/`acquire` 配对。`relaxed` 不阻止重排，不建立可见性保证。

### 错误 2：自旋不加 pause

```cpp
// 不理想：裸自旋——浪费 CPU + 总线争用
while (!ready.load(std::memory_order_acquire)) {}
// CPU 100% 占用，可能拖慢生产者（总线锁/缓存争用）

// 更好：加 pause 或 yield
while (!ready.load(std::memory_order_acquire)) {
    _mm_pause();  // x86 pause 指令，减少功耗和总线争用
    // 或 std::this_thread::yield();  // 让出 CPU 给其他线程
}
```

### 错误 3：发布后修改数据

```cpp
// 错误：发布后修改数据——读者可能读到部分新部分旧
data.field1 = 1;
data.field2 = 2;
ready.store(true, std::memory_order_release);

// 读者看到 ready=true 后：
// 但如果发布者继续修改 data：
data.field1 = 10;  // 读者可能看到 field1=10 但 field2=2
// 非原子数据的并发读写是 UB
```

**修复**：发布后不修改数据（用 const），或用序列号 + 双缓冲。

## 和 C 的区别

| 特性 | C (mutex) | C++ (atomic<bool>) |
|------|-----------|---------------------|
| 同步方式 | mutex + cv | atomic store/load |
| 内存屏障 | 隐式（mutex） | 显式（release/acquire） |
| 开销 | mutex lock/unlock ~20-100ns | atomic load/store ~1-10ns |
| 适用场景 | 多写者多读者 | 一次性发布 |
| 样板代码 | 多 | 少 |

## HFT 关联

- **SPSC 队列的基础**：生产者写数据 + release 存序列号；消费者 acquire 读序列号后读数据——HFT SPSC 无锁队列的经典模式。
- **行情发布**：行情线程写行情数据 + release 存序列号；策略线程 acquire 读序列号后读行情——无锁传递，延迟 ~10ns。
- **自旋 vs park**：HFT 热路径用 `atomic<bool>` 自旋（`_mm_pause`），不用 `condition_variable`（park 延迟 ~1-10μs）。

## 代码自测

### Q1: 下列代码保证 data==42 吗？

```cpp
std::atomic<bool> ready{false};
int data = 0;

// 线程 1
data = 42;
ready.store(true, std::memory_order_release);

// 线程 2
while (!ready.load(std::memory_order_acquire)) {}
std::cout << data;
```

<details>
<summary>答案与复习指引</summary>

**保证**。`release` store + `acquire` load 建立了 happens-before：
- `release` 阻止 `data=42` 被重排到 `store` 之后
- `acquire` 阻止 `cout << data` 被重排到 `load` 之前
- 线程 2 读到 `ready==true` 后，`data` 一定是 42

这是无锁同步的最简形式——不需要 mutex。

复习：release/acquire 配对 = 无锁版的 mutex。适合一次性发布。
</details>

### Q2: 如果把 release 改成 relaxed，会发生什么？

```cpp
data = 42;
ready.store(true, std::memory_order_relaxed);

while (!ready.load(std::memory_order_acquire)) {}
std::cout << data;
```

<details>
<summary>答案与复习指引</summary>

**不保证 data==42**。`relaxed` store 不阻止 `data=42` 被重排到 `store` 之后——线程 2 可能看到 `ready=true` 但 `data` 仍是旧值。

即使 load 端用 `acquire`，如果 store 端不用 `release`，也不建立 synchronizes-with 关系。

修复：store 必须用 `release`。acquire/release 必须配对。

复习：release 和 acquire 必须配对使用。一端不对就不建立 happens-before。
</details>

### Q3: 下列序列号模式有什么优势？

```cpp
std::atomic<uint32_t> seq{0};
Data data;

// 写者
data = new_data;
seq.store(seq.load(relaxed) + 1, std::memory_order_release);

// 读者
uint32_t s1 = seq.load(std::memory_order_acquire);
Data d = data;
uint32_t s2 = seq.load(std::memory_order_acquire);
if (s1 != s2) /* 重读 */;
```

<details>
<summary>答案与复习指引</summary>

优势：
1. **支持多重发布**：序列号每次递增，读者可以检测到多次更新——比 `atomic<bool>` 更灵活。
2. **无锁读取**：读者不需要锁，只需两次 atomic load + 比较——延迟 ~10ns。
3. **一致性保证**：如果 s1==s2，说明在读数据期间没有写者修改——读到的是一致快照。
4. **适合读多写少**：多个读者可以同时读，互不阻塞——比 `shared_mutex` 更轻量。

劣势：写者频繁更新时读者可能多次重试——适合写频率低的场景。

复习：序列号模式 = 无锁版的读写锁。HFT 行情发布的经典模式。
</details>

### Q4: 为什么 HFT 自旋用 `_mm_pause()` 而不是 `yield()`？

<details>
<summary>答案与复习指引</summary>

`_mm_pause()`（x86 `pause` 指令）：
- ~1 周期开销
- 提示 CPU"我在自旋"——减少流水线功耗
- 避免内存顺序违例（Memory Ordering Violation）惩罚
- 不让出 CPU——保持 cache 热度

`std::this_thread::yield()`：
- ~100-1000ns 开销（系统调用）
- 让出 CPU 给其他线程——可能丢失 cache
- 适合长等待（微秒+）

HFT 热路径自旋等待时间通常 < 1μs——用 `_mm_pause()` 保持低延迟 + cache 热度。`yield()` 适合非热路径的长等待。

复习：短自旋用 `_mm_pause()`，长等待用 `yield()` 或 `condition_variable`。
</details>

---

## 参考与延伸

- 下一节：[5.6 volatile ≠ atomic](06-volatile-not-atomic.md)
- 回到：[第 5 章](README.md)
