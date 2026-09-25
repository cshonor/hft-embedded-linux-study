# 8.2 数据并行：分块处理

> 第 8 章 · 上一节：[8.1 并行划分策略](01-partitioning.md) · 下一节：[8.3 减少共享：线程局部化](03-thread-local.md)

## 这节讲什么

数据并行的核心是"分块"——把大数组切成 N 块，每块一个线程处理。本节讲分块策略（均匀分块 vs 循环分块）、负载均衡、以及最后合并结果时的同步点设计。

---

## 核心规则（代码+表格）

### 均匀分块 vs 循环分块

```cpp
// 均匀分块：每线程连续处理一块
void block_partition(const std::vector<int>& v, int num_threads) {
    size_t block = v.size() / num_threads;
    std::vector<int> partial(num_threads, 0);
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        size_t start = i * block;
        size_t end = (i == num_threads - 1) ? v.size() : start + block;
        threads.emplace_back([&, i, start, end]{
            for (size_t j = start; j < end; ++j) partial[i] += v[j];
        });
    }
    for (auto& t : threads) t.join();
    int result = std::accumulate(partial.begin(), partial.end(), 0);
}

// 循环分块（round-robin）：线程 i 处理 i, i+N, i+2N, ...
void cyclic_partition(const std::vector<int>& v, int num_threads) {
    std::vector<int> partial(num_threads, 0);
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]{
            for (size_t j = i; j < v.size(); j += num_threads)
                partial[i] += v[j];
        });
    }
    for (auto& t : threads) t.join();
    int result = std::accumulate(partial.begin(), partial.end(), 0);
}
```

### 两种分块的对比

| 分块方式 | cache 友好 | 负载均衡 | 适用 |
|----------|-----------|----------|------|
| 均匀分块 | 好（连续访问） | 差（如果某块数据耗时不均） | 各元素耗时相同 |
| 循环分块 | 差（跳跃访问） | 好（耗时差异被均摊） | 各元素耗时差异大 |

### 负载不均的例子

```cpp
// 反例：质数检查，大数比小数慢得多
bool is_prime(int n);
// 均匀分块：线程0 处理 1-1000（快），线程3 处理 3001-4000（慢）
// → 线程0 空等线程3
// 循环分块：每线程处理 1,5,9... / 2,6,10... / 耗时被均摊
```

### 结果合并的同步点

```cpp
// 方式1：join 同步 + 主线程合并
for (auto& t : threads) t.join();   // 所有线程完成
result = accumulate(partial);        // 主线程合并

// 方式2：原子累加（无 join，但原子竞争）
std::atomic<int> result{0};
for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, i]{
        int local = 0;
        for (size_t j = i*block; j < (i+1)*block; ++j) local += v[j];
        result.fetch_add(local, std::memory_order_relaxed);  // 只一次原子操作
    });
}
// 方式2 更好：每线程只做一次 fetch_add，而非每元素一次
```

### 分块大小选择

| 块大小 | 优点 | 缺点 |
|--------|------|------|
| 大（= 总量/核数） | cache 友好、线程开销小 | 负载不均 |
| 小（如 4096 元素） | 负载均衡好 | 线程同步频繁 |
| 自适应 | 兼顾两者 | 实现复杂 |

---

## 新手要点（和 C 的区别）

- **C 程序员习惯均匀分块**：`#pragma omp parallel for` 默认就是均匀分块。但 C 程序员可能不知道循环分块（`schedule(dynamic)`）在负载不均时更好。C++ 手写时可以自由选择。
- **原子累加的技巧**：C 程序员可能直接 `result += v[j]`（非原子，数据竞争！）或每元素 `__sync_fetch_and_add`（太慢）。正确做法是每线程局部累加，最后只做一次原子合并——这在 C 和 C++ 里思路相同，但 C++ 的 `atomic` 更清晰。
- **cache 友好性是 C 程序员熟悉的**：均匀分块的连续访问对 cache 友好，这个概念 C 程序员懂（数组遍历比链表快）。但循环分块会破坏这个优势——要权衡。
- **`std::accumulate` 合并**：C 里用 `for` 循环手动累加 partial 数组，C++ 用 `std::accumulate` 更简洁。这只是语法糖，性能等价。

---

## HFT 关联

- **HFT 热路径极少用数据并行**：生产交易中数据量小（一个 tick），不值得分块。数据并行主要用于盘后批处理（回测、因子计算）。
- **盘后回测是数据并行的典型场景**：全市场 5000 只股票 × 240 分钟 × 250 天 = 3 亿条记录。按标的或时间分块，多线程并行回测——均匀分块 + join 合并。
- **循环分块用于因子计算**：不同标的的因子计算耗时差异大（如小盘股数据少、大盘股数据多），循环分块让耗时均摊。
- **原子合并只做一次**：HFT 批处理中，每线程用局部变量累加，最后 `fetch_add` 一次——避免每元素的原子操作开销。

---

## 自测题

1. 均匀分块和循环分块各有什么优缺点？什么时候选循环分块？
2. 质数检查（大数比小数慢）应该用哪种分块？为什么？
3. 为什么"每元素做一次原子 add"比"每线程局部累加 + 最后一次原子合并"慢？
4. 分块大小选太大或太小各有什么问题？
5. HFT 的哪个场景适合用数据并行？热路径适合吗？

<details>
<summary>参考答案</summary>

1. 均匀连续分块开销低、局部性好，但每块耗时不均时会让线程空等。
   循环分块按线程交错分配元素，能摊平与索引相关的负载差异，但连续局部性通常较差。
2. 若输入按大小排列且大数检查明显更慢，连续均匀分块会让后段线程承担更多工作。
   循环分块可把大小不同的数分散给各线程；更一般的不规则成本也可用动态小块调度。
3. 每次 atomic add 都对同一 cache line 做 RMW，造成序列化和跨核 cache line bouncing。
   局部累加只访问线程私有缓存，最终每线程合并一次，把共享原子操作从每元素一次降为每线程一次。
4. 块太大导致并行度不足和负载不均，最慢块决定完成时间。
   块太小则调度、队列和合并开销变大，缓存局部性也可能下降，需要按 workload 基准选阈值。
5. 大规模回测、盘后风险/指标计算和批量行情清洗适合数据并行。
   单条行情到订单的延迟热路径通常不适合通用分块调度；若批量固定且测量证明有益，可用专门 SIMD/固定分片。

</details>

---

## 参考与延伸

- 下一节：[8.3 减少共享：线程局部化](03-thread-local.md)
- 上一节：[8.1 并行划分策略](01-partitioning.md)
- 回到：[第 8 章](README.md)
