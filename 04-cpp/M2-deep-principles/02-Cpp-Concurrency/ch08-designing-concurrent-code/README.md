# 第 8 章 设计并发代码

**Designing Concurrent Code**

## 本章讲什么

前几章讲"怎么写正确的并发原语"，本章讲"怎么把任务拆分到多线程"。讲数据并行 vs 任务并行、划分策略（按数据/按任务/按流水线）、减少共享的可扩展性设计、以及并发异常安全的处理。

## 要点

### 并行划分策略

| 策略 | 思路 | 适用 |
|------|------|------|
| 数据并行 | 同一操作分摊到不同数据块 | 向量加法、批量行情处理 |
| 任务并行 | 不同操作分到不同线程 | 采集→解析→策略→下单流水线 |
| 流水线 | 数据流过多个阶段，每阶段一线程 | HFT 行情处理流水线 |
| 递归分治 | divide-and-conquer + fork/join | 快速排序、归并排序并行化 |

### 数据并行：分块处理

```cpp
// 并行累加：把数组分 N 块，每块一个线程
void parallel_sum(const std::vector<int>& v, int& result, size_t num_threads) {
    std::vector<int> partial(num_threads, 0);
    std::vector<std::thread> threads;
    size_t block = v.size() / num_threads;
    for (size_t i = 0; i < num_threads; ++i) {
        size_t start = i * block;
        size_t end = (i == num_threads - 1) ? v.size() : start + block;
        threads.emplace_back([&, i, start, end]{
            for (size_t j = start; j < end; ++j) partial[i] += v[j];
        });
    }
    for (auto& t : threads) t.join();
    result = std::accumulate(partial.begin(), partial.end(), 0);
}
```

### 减少共享：线程局部化

并发扩展性的头号敌人是**共享可变状态**。减少共享的手段：

1. **线程私有副本**：每个线程独立计数，最后合并（如上例 `partial`）。
2. **`thread_local` 存储**：每个线程一份独立实例。
```cpp
thread_local std::mt19937 rng;   // 每线程独立随机数发生器，无需锁
```
3. **不可变共享**：只读数据全局共享（盘前初始化的合约表、参数表）。
4. **无共享架构**：每个线程独占一段数据，靠消息传递通信（actor 模型）。

### 异常安全

线程函数抛异常会导致 `std::terminate`（默认未捕获行为）。处理方式：
- 线程内 `try/catch` 捕获，通过 `promise::set_exception` 传递。
- 用 `std::async`：返回的 future 会在 `get()` 时重抛异常。
- **绝不能让异常逃逸线程函数**。

### 可扩展性法则

阿姆达尔定律：
```
加速比 = 1 / ((1 - P) + P/N)
```
其中 P 是可并行比例，N 是核数。**串行部分（锁、共享资源）是扩展上限**——1% 的串行在 100 核上只能加速到 50 倍。

### 负载均衡

- **静态划分**：事先分好块，适合均匀工作量。
- **动态划分**：用任务队列，线程取一个做一个（work-stealing）。适合不均匀工作量。
- **C++17 并行算法**：`std::for_each(std::execution::par, ...)` 自动划分。

## HFT 关联

- **流水线架构**：HFT 经典布局——网卡收包线程 → 解析线程 → 策略线程 → 下单线程，每阶段绑独立核，用 SPSC 队列串联。每阶段无共享，扩展性近乎线性。
- **绑核 + `thread_local`**：每个策略线程独占一核，`thread_local` 存策略上下文，零跨核共享。
- **不可变快照**：行情字典盘前构建为 `const`，运行期只读不锁。
- **无共享订单路由**：每个合约分配到固定策略线程，按合约 ID hash 分流，同合约永远在同一线程处理——天然无竞争。
- **阿姆达尔定律的警示**：HFT 串行部分（如全局风控检查）是延迟下限，要尽量消除或并行化。
- **动态负载均衡慎用**：work-stealing 有任务队列原子操作开销，HFT 热路径倾向静态划分（确定性延迟）。

## 自测题

1. 数据并行和任务并行的区别是什么？HFT 行情处理流水线属于哪种？
2. 减少共享可变状态的四种手段是什么？`thread_local` 如何帮助扩展性？
3. 阿姆达尔定律是什么？为什么 1% 的串行在 100 核上只能加速到 50 倍？
4. 线程函数抛异常会怎样？正确的处理方式是什么？
5. HFT 为什么倾向静态划分而非 work-stealing？确定性延迟如何保证？

## 代码自测

### Q1: false sharing
```cpp
struct Counters {
    int a;  // 线程 A 写
    int b;  // 线程 B 写
};

Counters c;
std::thread t1([&] { for(int i=0;i<1000000;++i) ++c.a; });
std::thread t2([&] { for(int i=0;i<1000000;++i) ++c.b; });
```
> 两个线程分别写不同变量，为什么性能很差？怎么修复？

<details>
<summary>答案与复习指引</summary>

**False sharing**：`a` 和 `b` 在同一个 cache line（64 字节）内。线程 A 写 `a` 使该 cache line 在 A 的核上变脏，线程 B 写 `b` 需要先 invalidate A 的 cache line → **cache line ping-pong**。虽然写不同变量，但硬件层面在同一个 cache line 上争抢。

**修复**：用 `alignas(64)` 让每个变量独占一个 cache line：
```cpp
struct Counters {
    alignas(64) int a;
    alignas(64) int b;
};
```

**HFT**：性能计数器、per-thread 状态必须避免 false sharing。

**复习：** → [false sharing](./README.md)
</details>
