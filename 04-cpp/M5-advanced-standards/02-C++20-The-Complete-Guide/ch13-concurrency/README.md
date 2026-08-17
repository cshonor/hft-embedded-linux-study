# 第 13 章 并发特性

**Concurrency Features**

## 本章讲什么

C++20 的并发增强：`latch`/`barrier`/`semaphore` 同步原语、`atomic_ref`、`atomic<shared_ptr>`、`jthread`（第 12 章）、`std::stop` 机制、协程（第 14-15 章）。（深入并发见 [2-Cpp-Concurrency](../../../M3-deep-principles/02-Cpp-Concurrency/)。）

## 要点

### `latch`：一次性计数屏障

```cpp
#include <latch>

std::latch ready(N_THREADS);

for (int i = 0; i < N_THREADS; ++i) {
    threads.emplace_back([&]{
        init();
        ready.count_down();   // -1
    });
}
ready.wait();   // 等所有线程 count_down
// 所有线程初始化完成
```

`latch` 不可重置——一次性的。所有线程到齐后开闸。

### `barrier`：可复用屏障

```cpp
#include <barrier>

std::barrier sync(N_THREADS, []() noexcept {
    // 每阶段结束回调（可空）
});

for (int phase = 0; phase < N_PHASES; ++phase) {
    process_phase(phase);
    sync.arrive_and_wait();   // 到齐后继续下一阶段
}
```

`barrier` 可复用——每轮 `arrive_and_wait` 后自动重置，适合分阶段并行。

### `counting_semaphore` / `binary_semaphore`

```cpp
#include <semaphore>

// 计数信号量：控制并发数
std::counting_semaphore<4> sem(4);   // 最多 4 并发

void worker() {
    sem.acquire();   // -1（0 时阻塞）
    do_work();
    sem.release();   // +1
}

// 二值信号量（类似 mutex 但可非所有者释放）
std::binary_semaphore signal(0);

// 生产者
signal.release();   // 通知
// 消费者
signal.acquire();   // 等待
```

信号量比 mutex 灵活——可非所有者释放，适合生产者-消费者通知。

### `std::atomic_ref`：对已有变量的原子引用

```cpp
int counter = 0;   // 普通变量

// atomic_ref：不修改变量类型，临时原子访问
std::atomic_ref<int> ref(counter);
ref.fetch_add(1);

// counter 仍是普通 int，但通过 ref 访问是原子的
```

用于"已有非原子变量，临时需要原子访问"——不用改变量声明。

### `std::atomic<std::shared_ptr>`

```cpp
// C++20 前：shared_ptr 的原子操作要用 free function
std::shared_ptr<T> p;
std::atomic_store(&p, std::make_shared<T>());

// C++20：atomic<shared_ptr>
std::atomic<std::shared_ptr<T>> ap;
ap.store(std::make_shared<T>());
auto p2 = ap.load();
```

`atomic<shared_ptr>` 让无锁的共享指针读写标准化，替代旧的 `atomic_load/shared_ptr` free function。

### `std::atomic_flag::wait` / `notify`

```cpp
std::atomic_flag flag;

// 线程 A
flag.wait(false);   // 等 flag 变 true
// 线程 B
flag.test_and_set();
flag.notify_one();  // 唤醒
```

C++20 的 atomic `wait`/`notify` 让原子变量本身能做条件等待，不需要 `condition_variable` + mutex。

## HFT 关联

- **`latch` 做阶段同步**：策略初始化多阶段用 `latch` 等所有 worker 就位再开闸。
- **`barrier` 分阶段并行**：回测分片处理用 `barrier` 同步各分片完成，再聚合。
- **`binary_semaphore` 做通知**：生产者-消费者用 `binary_semaphore` 替代 mutex+cv，更简洁。
- **`atomic_ref` 适配旧代码**：已有 `int counter` 不用改成 `atomic<int>`，用 `atomic_ref` 临时原子访问。
- **`atomic<shared_ptr>` 配置热更新**：配置对象用 `atomic<shared_ptr<Config>>`，写者 `store` 新配置，读者 `load` 无锁读。
- **`atomic::wait/notify` 替代 cv**：SPSC 队列用 `atomic::wait` 等序列号更新，比 mutex+cv 轻量。但热路径仍倾向自旋（避免 park 开销）。

## 自测题

1. `latch` 和 `barrier` 的区别？分别适合什么场景？
2. `binary_semaphore` 和 mutex 的区别？为什么说可"非所有者释放"？
3. `atomic_ref` 解决什么问题？
4. `atomic<shared_ptr>` 相比旧的 `atomic_load(&ptr)` 好在哪？
5. HFT 配置热更新如何用 `atomic<shared_ptr<Config>>`？

## 代码自测

### Q1: 新同步原语
```cpp
// C++20: counting_semaphore
std::counting_semaphore<3> sem(3);  // 最多 3 个线程同时进入
sem.acquire();  // 计数 -1，如果为 0 则阻塞
sem.release();  // 计数 +1，唤醒一个等待线程

// binary_semaphore = counting_semaphore<1>
std::binary_semaphore start(0);
// 线程 A
start.acquire();  // 阻塞直到线程 B release
// 线程 B
start.release();  // 释放信号

// latch: 一次性同步点
std::latch done(3);  // 等 3 个线程
done.count_down();   // 计数 -1
done.wait();         // 阻塞到计数归零

// barrier: 可重复使用的同步点
std::barrier sync(3, [] { /* 所有线程到达后执行 */ });
sync.arrive_and_wait();  // 到达并等待
```
> semaphore/latch/barrier 分别解决什么同步问题？

<details>
<summary>答案与复习指引</summary>

| 原语 | 用途 | 可复用 |
|------|------|--------|
| `counting_semaphore<N>` | 限制并发数（如连接池上限） | ✅ |
| `binary_semaphore` | 互斥/信号传递（类似互斥锁） | ✅ |
| `latch` | 一次性等待 N 个线程到达 | ❌（一次性） |
| `barrier` | 多阶段并行——每阶段等所有线程 | ✅ |

**场景**：
- **semaphore**：限制同时访问资源的线程数（如最多 3 个线程读文件）
- **latch**：初始化阶段——主线程等所有工作线程完成初始化
- **barrier**：并行计算——每轮迭代等所有线程完成上一轮

**HFT**：barrier 用于多核并行回测的同步点。热路径不用（阻塞开销大）。

**复习：** → [新同步原语](./README.md)
</details>
