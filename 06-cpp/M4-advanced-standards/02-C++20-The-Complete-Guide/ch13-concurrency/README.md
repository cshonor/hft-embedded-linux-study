# 第 13 章 并发特性

**Concurrency Features**

## 本章讲什么

C++20 的并发增强：`latch`/`barrier`/`semaphore` 同步原语、`atomic_ref`、`atomic<shared_ptr>`、`jthread`（第 12 章）、`std::stop` 机制、协程（第 14-15 章）。（深入并发见 [02-Cpp-Concurrency](../../../M2-deep-principles/02-Cpp-Concurrency/)。）

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
