# D.7 latch / barrier 屏障

> 附录 D · 上一节：[D.6 semaphore 信号量](06-semaphore.md) · 下一节：[D.8 memory_order 内存序](08-memory-order.md)

## 这节讲什么

`<latch>` 和 `<barrier>`（C++20）提供线程屏障——让多个线程在某个点同步。本节是速查参考——`latch`（一次性）和 `barrier`（可重用）的区别和用法。

---

## 核心规则（代码+表格）

### `latch`：一次性屏障

| 接口 | 说明 |
|------|------|
| `latch(n)` | 初始计数 n |
| `count_down(n=1)` | 计数 -n |
| `wait()` | 等待计数归零 |
| `arrive_and_wait()` | -1 并等待归零 |
| `try_wait()` | 计数是否为 0 |

```cpp
#include <latch>

std::latch start_latch(4);  // 4 个线程

void worker(int id) {
    // 准备工作
    std::cout << "worker " << id << " ready\n";
    start_latch.arrive_and_wait();  // 等所有线程就绪
    // 同时开始
    std::cout << "worker " << id << " started\n";
}

std::vector<std::thread> threads;
for (int i = 0; i < 4; ++i)
    threads.emplace_back(worker, i);
for (auto& t : threads) t.join();
// 输出：所有 "ready" 后才出现 "started"
```

### `barrier`：可重用屏障

| 接口 | 说明 |
|------|------|
| `barrier(n, completion)` | 初始 n，归零时调用 completion |
| `arrive_and_wait()` | -1 并等待下一阶段 |
| `arrive(n=1)` | -n 不等待 |
| `wait(phase)` | 等待当前阶段完成 |

```cpp
#include <barrier>

// 每阶段结束后打印
std::barrier sync_point(4, []() noexcept {
    std::cout << "--- phase complete ---\n";
});

void worker(int id) {
    for (int phase = 0; phase < 3; ++phase) {
        do_phase_work(id, phase);
        sync_point.arrive_and_wait();  // 等所有线程完成本阶段
        // 所有线程同时进入下一阶段
    }
}
// 每阶段 4 个线程都完成后，打印 "--- phase complete ---"
// barrier 自动重置，可重复使用
```

### `latch` vs `barrier`

| 维度 | latch | barrier |
|------|-------|---------|
| 重用 | 一次性（用完即弃） | 可重用（自动重置） |
| 等待 | `wait()` 可多次调用 | `wait(phase)` 等特定阶段 |
| 完成回调 | 无 | 有（completion 函数） |
| 适用 | 一次性同步（初始化） | 多阶段同步（流水线） |
| 计数方向 | 只减不增 | 归零后自动重置 |

### 应用：初始化同步

```cpp
// 所有线程初始化完成后才开始交易
std::latch init_done(NUM_THREADS);

void trading_thread(int id) {
    init_thread(id);        // 初始化本线程的数据
    init_done.arrive_and_wait();  // 等所有线程初始化完
    start_trading(id);      // 同时开始交易
}
```

### 应用：多阶段流水线

```cpp
// 流水线：每阶段所有线程完成后才进入下一阶段
std::barrier phase_sync(NUM_THREADS);

void pipeline_worker(int id) {
    for (int phase = 0; phase < NUM_PHASES; ++phase) {
        // 阶段1：各自处理
        process_phase(id, phase);
        
        // 同步：等所有线程完成本阶段
        phase_sync.arrive_and_wait();
        
        // 阶段2：交换数据（所有线程都在同一点）
        exchange_data(id, phase);
        
        phase_sync.arrive_and_wait();
    }
}
```

### 应用：completion 回调做汇总

```cpp
// 每轮计算后，由一个线程汇总结果
std::barrier compute_barrier(NUM_THREADS, [&]() noexcept {
    // 这个回调在每轮归零时由一个线程执行
    aggregate_results();
    publish_snapshot();
});

void compute_worker(int id) {
    for (int round = 0; round < N; ++round) {
        local_results[id] = compute(id, round);
        compute_barrier.arrive_and_wait();  // 触发汇总
    }
}
```

---

## 新手要点（和 C 的区别）

- **C 没有 latch/barrier**：C 程序员要手写屏障（mutex + cv + 计数器）——繁琐且容易出错。C++20 的 `latch`/`barrier` 是标准设施。
- **POSIX 有 `pthread_barrier_t`**：C 程序员如果用过 `pthread_barrier_wait`——`std::barrier` 类似但更强大（有 completion 回调）。`latch` 是 POSIX 没有的新概念。
- **`latch` 一次性 vs `barrier` 可重用**：C 程序员可能不理解为什么要分两个——`latch` 更轻（无需重置），`barrier` 更通用（自动重置）。一次性同步用 `latch`，循环同步用 `barrier`。
- **completion 回调是 `barrier` 的亮点**：C 的 `pthread_barrier` 没有 completion 回调——C++ 的 `barrier` 可以在每轮归零时执行汇总逻辑，无需额外同步。

---

## HFT 关联

- **HFT 系统启动用 `latch`**：所有交易线程初始化完成 → `latch.arrive_and_wait()` → 同时开始交易——确保所有线程在同一时刻启动，避免"先启动的线程拿到旧数据"。
- **多阶段计算用 `barrier`**：HFT 盘后的多因子计算——每阶段（每个因子）所有线程完成后才算下一个因子——`barrier` 保证一致性。
- **completion 回调做快照发布**：HFT 的多线程计算完成后，通过 `barrier` 的 completion 回调发布快照——一个线程做汇总，无需额外锁。
- **HFT 热路径不用屏障**：`barrier` 等待时阻塞——HFT 热路径用 SPSC 队列，无屏障。

---

## 自测题

1. `latch` 和 `barrier` 的主要区别是什么？
2. `barrier` 的 completion 回调在什么时候执行？由哪个线程执行？
3. 如何用 `latch` 实现"所有线程初始化完成后同时开始"？
4. `barrier` 如何实现"多阶段流水线同步"？
5. 为什么 HFT 系统启动用 `latch` 而热路径不用 `barrier`？

---

## 参考与延伸

- 下一节：[D.8 memory_order 内存序](08-memory-order.md)
- 上一节：[D.6 semaphore 信号量](06-semaphore.md)
- 回到：[附录 D](README.md)
