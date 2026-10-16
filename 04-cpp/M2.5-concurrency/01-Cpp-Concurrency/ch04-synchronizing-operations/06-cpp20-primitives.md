# 4.6 C++20 新同步原语

> 第 4 章 · 上一节：[4.5 超时等待](05-timeout.md) · 下一章：[第 5 章 内存模型和原子操作](../ch05-memory-model-atomics/README.md)

## 这节讲什么

C++20 引入 `latch`、`barrier`、`semaphore`——标准化的同步原语，替代手写 mutex+cv 组合。

## 为什么要学这个（先建立直觉）

C 程序员在 C++20 之前要手写"等所有线程就位再开跑"的同步：

```c
// C：手写 barrier——mutex + cv + 计数器
typedef struct {
    int count;
    int target;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
} Barrier;

void barrier_wait(Barrier* b) {
    pthread_mutex_lock(&b->mtx);
    b->count++;
    if (b->count == b->target) {
        b->count = 0;
        pthread_cond_broadcast(&b->cv);  // 唤醒所有
    } else {
        while (pthread_cond_wait(&b->cv, &b->mtx) != 0) {}
    }
    pthread_mutex_unlock(&b->mtx);
}
// 大量样板代码，容易出错
```

C++20 一行搞定：

```cpp
// C++20：latch——一次性计数屏障
std::latch start(3);  // 等 3 个线程就位

void worker() {
    prepare();
    start.count_down();  // 准备好了
    start.wait();        // 等所有人准备好
    run();               // 一起开跑
}
```

## 四个新原语详解

### 1. std::latch：一次性计数屏障

```cpp
#include <latch>

std::latch latch(4);  // 等 4 个线程

void worker(int id) {
    init(id);
    latch.count_down();  // 计数减一
    // 不需要等——count_down 后可以继续做其他事
}

// 主线程等所有 worker 完成 init
for (int i = 0; i < 4; i++)
    std::thread(worker, i).detach();
latch.wait();  // 阻塞直到计数归零
// 所有 worker 都完成了 init

// 特点：
// - 一次性：归零后不可重置
// - count_down 可以减任意值：latch.count_down(2)
// - 可以在 wait 的同时 count_down（try_wait 检查是否归零）
```

### 2. std::barrier：可复用屏障

```cpp
#include <barrier>

std::barrier sync_point(4, []() noexcept {
    std::cout << "phase complete\n";
    // 在所有线程释放前的回调
});

void worker(int id) {
    for (int phase = 0; phase < 3; phase++) {
        do_phase_work(id, phase);
        sync_point.arrive_and_wait();  // 等所有线程完成本阶段
        // 所有线程一起进入下一阶段
    }
}
// 特点：
// - 可复用：每次 arrive_and_wait 后自动重置计数
// - 支持回调：每阶段结束时执行
// - 适合分阶段并行（map-reduce 风格）
```

### 3. std::counting_semaphore：信号量

```cpp
#include <semaphore>

// 限制同时访问的资源数（如数据库连接池）
std::counting_semaphore<10> db_slots(5);  // 最多 5 个并发

void query_db() {
    db_slots.acquire();  // 获取一个信号量（如果已满则阻塞）
    do_db_work();
    db_slots.release();  // 释放
}

// 特点：
// - acquire/release 可以在不同线程（不像 mutex 必须同一线程）
// - 模板参数是最大值（编译期检查）
// - 运行时初始值可以小于最大值
```

### 4. std::binary_semaphore：二值信号量

```cpp
std::binary_semaphore sem(0);  // 初始不可用

// 线程 1：等待信号
sem.acquire();  // 阻塞直到 sem 被 release
do_work();

// 线程 2：发信号
prepare_data();
sem.release();  // 通知线程 1 可以开始

// 特点：
// - 等价于 counting_semaphore<1>
// - 类似 mutex 但可由非所有者释放
// - 适合"通知"场景（mutex 不适合，因为 lock/unlock 必须同线程）
```

| 原语 | 作用 | 可复用 | 典型场景 |
|------|------|--------|----------|
| `latch` | 一次性计数屏障 | 否 | 等初始化完成 |
| `barrier` | 可复用屏障 | 是 | 分阶段并行 |
| `counting_semaphore` | 控制并发数 | 是 | 资源池/限流 |
| `binary_semaphore` | 二值通知 | 是 | 线程间通知 |

## 常见错误（新手踩坑）

### 错误 1：用 latch 做可复用同步

```cpp
// 错误：latch 是一次性的
std::latch sync(3);
void loop() {
    work();
    sync.count_down();
    sync.wait();  // 第一次 OK
    // 第二次调用 loop()：latch 已归零，wait 立即返回——不同步！
}
```

**修复**：可复用场景用 `std::barrier`。

### 错误 2：barrier 的回调抛异常

```cpp
// 错误：barrier 回调抛异常——UB
std::barrier b(3, []() {
    throw std::runtime_error("oops");  // UB！
});
```

**修复**：barrier 回调必须是 `noexcept` 的——不能抛异常。

### 错误 3：semaphore 计数错误

```cpp
// 错误：release 次数 > acquire 次数——计数溢出
std::counting_semaphore<5> sem(0);
sem.release();  // count = 1
sem.release();  // count = 2
// 如果只有一个线程 acquire，另一个 release 是多余的
// counting_semaphore 的计数不应超过模板参数
```

**注意**：`release()` 使计数加一，`acquire()` 使计数减一。计数不能超过编译期最大值。

## 和 C 的区别

| 特性 | C | C++20 |
|------|---|-------|
| 屏障 | `pthread_barrier_t`（POSIX） | `std::latch`/`std::barrier` |
| 信号量 | `sem_t`（POSIX） | `std::counting_semaphore` |
| 一次性同步 | 手写 mutex+cv+计数器 | `std::latch` |
| 可复用屏障 | `pthread_barrier_t` | `std::barrier`（带回调） |

## HFT 关联

- **latch 做阶段同步**：策略初始化多阶段用 `latch` 等所有 worker 就位再开闸——简洁且正确。
- **barrier 做批量处理**：分片行情处理用 `barrier` 同步各分片完成，再聚合——分阶段 map-reduce。
- **semaphore 做资源限流**：限制同时访问交易所的连接数——`counting_semaphore` 比手写 mutex+计数器更简洁。
- **binary_semaphore 做通知**：替代 `mutex + condition_variable` 的通知模式——一行 acquire/release 代替多行 mutex/cv/wait/notify。

## 代码自测

### Q1: 下列代码有什么问题？

```cpp
std::latch sync(3);

void worker() {
    for (int i = 0; i < 5; i++) {
        work();
        sync.count_down();
        sync.wait();
    }
}
// 3 个线程同时调用 worker
```

<details>
<summary>答案与复习指引</summary>

**latch 是一次性的**——第一次 `count_down` + `wait` 后归零，后续循环 `count_down` 在已归零的 latch 上是 UB，`wait` 立即返回不同步。

修复：用 `std::barrier`——可复用：
```cpp
std::barrier sync(3);
void worker() {
    for (int i = 0; i < 5; i++) {
        work();
        sync.arrive_and_wait();  // 可复用
    }
}
```

复习：latch 一次性，barrier 可复用。
</details>

### Q2: 下列代码能正确同步吗？

```cpp
std::binary_semaphore sem(0);

std::thread t([&] {
    sem.acquire();  // 等待信号
    std::cout << "running";
});

std::this_thread::sleep_for(std::chrono::milliseconds(100));
sem.release();  // 发信号
t.join();
```

<details>
<summary>答案与复习指引</summary>

**能正确同步**。`binary_semaphore(0)` 初始不可用，`acquire()` 阻塞直到 `release()` 被调用。

这比 `mutex + condition_variable` 简洁得多——不需要 mutex、不需要谓词、不需要 unique_lock。

但注意：`binary_semaphore` 不保证 FIFO 唤醒顺序——如果有多个 acquire 等待，release 唤醒哪个未指定。

复习：`binary_semaphore` 是"通知"场景的最佳选择——比 mutex+cv 简洁。
</details>

### Q3: barrier 的回调在什么时候执行？

```cpp
std::barrier b(3, []() noexcept {
    std::cout << "callback\n";
});

void worker() {
    for (int i = 0; i < 2; i++) {
        std::cout << "phase " << i << "\n";
        b.arrive_and_wait();
    }
}
// 3 个线程调用 worker
```

<details>
<summary>答案与复习指引</summary>

回调在**所有线程到达 barrier 时、释放前**执行。输出顺序：

```
phase 0  (3 个线程各打印一次)
callback (最后一个到达的线程执行回调)
phase 1  (所有线程被释放，进入下一阶段)
callback (第二阶段完成)
```

回调只在一个线程上执行（通常是最后一个到达的）。回调必须 `noexcept`。

复习：barrier 回调 = "阶段完成时的收尾工作"——聚合数据、打印日志等。
</details>

### Q4: 为什么 HFT 用 binary_semaphore 替代 mutex+cv 做通知？

<details>
<summary>答案与复习指引</summary>

| 方案 | 代码行数 | 开销 |
|------|----------|------|
| mutex + cv | 5-8 行（mutex + unique_lock + cv.wait + notify） | mutex lock/unlock + cv park/unpark |
| binary_semaphore | 2 行（acquire + release） | 原子操作（通常更轻量） |

binary_semaphore 的优势：
1. **简洁**：不需要 mutex、unique_lock、谓词
2. **可跨线程释放**：mutex 必须同线程 lock/unlock，semaphore 可以 A 线程 acquire、B 线程 release
3. **性能**：通常用原子操作实现，比 mutex+cv 更轻量

劣势：不能保护临界区（只是通知），不能带条件谓词。

复习：保护共享数据用 mutex，线程间通知用 binary_semaphore。
</details>

---

## 参考与延伸

- 下一章：[第 5 章 内存模型和原子操作](../ch05-memory-model-atomics/README.md)
- 回到：[第 4 章](README.md)
