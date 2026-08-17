# 8.1 并行划分策略

> 第 8 章 设计并发代码 · 上一章：[7.5 MPMC 的复杂度](../ch07-lock-free-containers/05-mpmc.md) · 下一节：[8.2 数据并行：分块处理](02-data-parallel.md)

## 这节讲什么

前 7 章讲"怎么写正确的并发原语"，本章讲"怎么把任务拆分到多线程"。本节讲四种并行划分策略——数据并行、任务并行、流水线、递归分治——以及它们各自适合什么场景。

---

## 核心规则（代码+表格）

### 四种划分策略

| 策略 | 思路 | 适用 | HFT 示例 |
|------|------|------|----------|
| **数据并行** | 同一操作分摊到不同数据块 | 向量运算、批量处理 | 批量行情解析、多标的回测 |
| **任务并行** | 不同操作分到不同线程 | 采集→解析→策略→下单 | 行情采集、策略计算、下单分离 |
| **流水线** | 数据流过多个阶段，每阶段一线程 | 流式处理 | HFT 行情处理流水线 |
| **递归分治** | divide-and-conquer + fork/join | 排序、搜索 | 并行快排、批量风控检查 |

### 数据并行 vs 任务并行

```cpp
// 数据并行：4 个线程各处理 1/4 的数据
void data_parallel(std::vector<int>& data) {
    size_t n = data.size() / 4;
    std::thread t1([&]{ process(data, 0, n); });
    std::thread t2([&]{ process(data, n, 2*n); });
    std::thread t3([&]{ process(data, 2*n, 3*n); });
    std::thread t4([&]{ process(data, 3*n, data.size()); });
    t1.join(); t2.join(); t3.join(); t4.join();
}

// 任务并行：4 个线程各做不同的事
void task_parallel() {
    MarketDataFeed feed;
    StrategyEngine strategy;
    OrderManager orders;
    RiskCheck risk;
    std::thread t1([&]{ feed.run(); });        // 采集行情
    std::thread t2([&]{ strategy.run(); });     // 策略计算
    std::thread t3([&]{ orders.run(); });       // 下单
    std::thread t4([&]{ risk.run(); });         // 风控
    // ...
}
```

### 流水线模式

```cpp
// 流水线：数据依次流过各阶段
// Stage1(采集) → Stage2(解析) → Stage3(策略) → Stage4(下单)
// 每个阶段一个线程，阶段间用 SPSC 队列连接

void pipeline() {
    spsc_queue<RawPacket> q1;   // 采集→解析
    spsc_queue<TickData> q2;    // 解析→策略
    spsc_queue<Order> q3;       // 策略→下单

    std::thread collector([&]{ while(running){ q1.push(capture()); } });
    std::thread parser([&]{ RawPacket p; while(q1.pop(p)) q2.push(parse(p)); });
    std::thread strategy([&]{ TickData t; while(q2.pop(t)) q3.push(compute(t)); });
    std::thread trader([&]{ Order o; while(q3.pop(o)) send(o); });
}
```

### 递归分治（fork/join）

```cpp
// 并行快速排序
template <typename T>
void parallel_quicksort(std::vector<T>& v) {
    if (v.size() <= 10000) {  // 阈值：小任务直接串行
        std::sort(v.begin(), v.end());
        return;
    }
    auto pivot = v[v.size() / 2];
    std::vector<T> left, right;
    for (auto& x : v) (x < pivot ? left : right).push_back(x);

    std::thread t1([&]{ parallel_quicksort(left); });
    parallel_quicksort(right);
    t1.join();

    v = std::move(left);
    v.push_back(pivot);
    v.insert(v.end(), right.begin(), right.end());
}
```

---

## 新手要点（和 C 的区别）

- **C 程序员通常只用任务并行**：C 里写多线程通常是"一个线程做网络、一个线程做计算"——这是任务并行。数据并行在 C 里要手动分块，C++17 的并行 STL（`std::execution::par`）让它更简洁（见第 10 章）。
- **流水线是 HFT 的核心模式**：C 程序员可能不熟悉"阶段间用队列连接"的流水线——但这是 HFT 的标准架构。每个阶段绑核，阶段间用 SPSC 队列零拷贝传递。
- **递归分治的阈值很重要**：C 程序员可能对每个小任务都开线程——但线程创建有 ~10-50μs 开销。任务太小时必须串行（阈值通常 1万-10万元素）。C++ 的 `std::thread` 开销和 pthread 类似。
- **数据并行 vs 任务并行要分清**：C 程序员容易混淆。数据并行是"同样的操作，不同的数据"；任务并行是"不同的操作"。混合使用时要明确每个线程的角色。

---

## HFT 关联

- **HFT 是流水线架构的典型**：采集（网卡线程）→ 解析 → 策略 → 风控 → 下单，每阶段一个线程绑核，阶段间 SPSC 队列。这是 HFT 的标准设计。
- **数据并行用于批量场景**：如盘后回测——把历史数据分块，多线程并行回测多个标的。生产交易通常不用数据并行（延迟敏感，不分块）。
- **递归分治极少用于 HFT 热路径**：fork/join 的线程同步开销对纳秒级系统太重。但如果做盘后批量分析（如全市场因子计算），并行 STL 比手写分治更好。
- **任务划分 = 核数绑定**：HFT 系统的线程数通常等于核数（或更少），每线程绑一个核——划分策略要和物理核一一对应，避免调度抖动。

---

## 自测题

1. 数据并行和任务并行有什么区别？各举一个 HFT 场景的例子。
2. 流水线模式为什么适合 HFT？阶段间用什么数据结构连接？
3. 递归分治（fork/join）为什么要设阈值？阈值太小会怎样？
4. 什么场景适合用数据并行？什么场景适合用任务并行？
5. HFT 系统的线程数通常怎么确定？为什么不是越多越好？

---

## 参考与延伸

- 下一节：[8.2 数据并行：分块处理](02-data-parallel.md)
- 上一章：[7.5 MPMC 的复杂度](../ch07-lock-free-containers/05-mpmc.md)
- 回到：[第 8 章](README.md)
