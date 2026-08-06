# 10.5 与传统手写并行的对比

> 第 10 章 · 上一节：[10.4 适合并行的算法](04-suitable-algorithms.md) · 下一章：[11.1 并发 bug 的特征](../ch11-testing-debugging/01-bug-characteristics.md)

## 这节讲什么

并行 STL 和手写 `std::thread` 分块各有利弊。本节对比两者在简洁性、性能、可控性、调试性上的差异，以及什么场景该用哪种。

---

## 核心规则（代码+表格）

### 同一任务的两种写法

```cpp
// 任务：并行计算向量平方和

// 写法1：手写 std::thread 分块
int manual_sqsum(const std::vector<int>& v) {
    int num_threads = std::thread::hardware_concurrency();
    size_t block = v.size() / num_threads;
    std::vector<int> partial(num_threads, 0);
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        size_t start = i * block;
        size_t end = (i == num_threads - 1) ? v.size() : start + block;
        threads.emplace_back([&, i, start, end]{
            for (size_t j = start; j < end; ++j)
                partial[i] += v[j] * v[j];
        });
    }
    for (auto& t : threads) t.join();
    return std::accumulate(partial.begin(), partial.end(), 0);
}

// 写法2：并行 STL
int stl_sqsum(const std::vector<int>& v) {
    return std::transform_reduce(
        std::execution::par,
        v.begin(), v.end(),
        0, std::plus<int>(),
        [](int x){ return x * x; }
    );
}
// 15 行 vs 3 行
```

### 对比表

| 维度 | 并行 STL | 手写 std::thread |
|------|---------|-----------------|
| 代码量 | 极少（1-3 行） | 多（15-30 行） |
| 正确性 | 标准保证 | 手动管理（易错） |
| 分块策略 | 实现自动选择 | 手动写死 |
| 负载均衡 | 实现自动（可能 work-stealing） | 手动（通常静态） |
| 线程管理 | 实现内部池化 | 手动创建/销毁 |
| 可控性 | 低（黑盒） | 高（完全可控） |
| 调试性 | 难（标准库内部） | 易（自己的代码） |
| 依赖 | 可能需 TBB | 仅标准库 |
| 适用 | 通用数据并行 | 特殊需求 |

### 什么时候手写

```cpp
// 场景1：需要精确控制线程数和绑核
// 并行 STL 不暴露线程池配置 → HFT 要绑核必须手写
void hft_worker() {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(3, &cpuset);  // 绑核 3
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    // ... 手写分块 ...
}

// 场景2：需要自定义同步（如流水线）
// 并行 STL 是 fork/join 模型 → 流水线要手写
// Stage1 → SPSC → Stage2 → SPSC → Stage3

// 场景3：特殊数据结构
// 并行 STL 作用于迭代器 → 自定义无锁结构要手写
void process_spsc_queue(spsc_queue<Tick>& q) {
    Tick t;
    while (q.pop(t)) process(t);  // 手写消费循环
}

// 场景4：极低延迟（纳秒级）
// 并行 STL 有任务调度开销 → HFT 热路径手写
```

### 什么时候用并行 STL

```cpp
// 场景1：批量数据处理（盘后）
int total_volume = std::transform_reduce(
    std::execution::par,
    ticks.begin(), ticks.end(),
    0, std::plus<>(),
    [](const Tick& t){ return t.volume; }
);

// 场景2：排序大数据
std::sort(std::execution::par, large_vec.begin(), large_vec.end());

// 场景3：简单的数据并行
std::for_each(std::execution::par, data.begin(), data.end(), process);

// 场景4：快速原型
// 不确定是否需要并行 → 先用 STL，benchmark 后再决定是否手写
```

---

## 新手要点（和 C 的区别）

- **C 没有并行 STL，只能手写**：C 程序员做并行要么 `pthread` 手写，要么 OpenMP 指令。C++17 的并行 STL 是巨大进步——3 行代码替代 30 行。
- **OpenMP 是中间方案**：`#pragma omp parallel for` 比 `pthread` 简洁，比 C++ STL 灵活度低。C 程序员如果用过 OpenMP，理解 C++ 并行 STL 会容易——都是"声明式并行"。
- **"可控性 vs 简洁性"的权衡**：C 程序员可能习惯完全可控（手写 pthread）——但 HFT 以外的场景，并行 STL 的简洁性和正确性更有价值。不要过度手写。
- **依赖问题**：C 程序员可能觉得"标准库就是零依赖"——但 C++17 并行 STL 的实现可能依赖 TBB（GCC）或 PPL（MSVC）。部署时要注意链接。

---

## HFT 关联

- **HFT 热路径手写，盘后用 STL**：这是 HFT 的通用原则。热路径（纳秒级）需要绑核、SPSC 队列、无调度抖动——必须手写。盘后批处理（毫秒级）用并行 STL——简洁高效。
- **"先 STL 后手写"的开发流程**：HFT 系统开发时，先用并行 STL 快速实现 → benchmark 找到瓶颈 → 只对瓶颈手写优化。避免过早优化。
- **绑核是手写的核心理由**：HFT 必须绑核消除调度抖动——并行 STL 不暴露线程亲和性设置。这是 HFT 选手写而非用 STL 的最主要原因。
- **TBB 依赖在 HFT 中的风险**：HFT 生产环境可能不想引入 TBB 依赖（版本管理、ABI 兼容）——这也是手写的一个理由。但盘后工具链可以接受 TBB。

---

## 自测题

1. 并行 STL 和手写 `std::thread` 分块在代码量上差多少？
2. 什么场景适合用并行 STL？什么场景必须手写？
3. 为什么 HFT 热路径不能直接用并行 STL？最主要的原因是什么？
4. "先 STL 后手写"的开发流程有什么好处？
5. 并行 STL 的实现可能依赖什么库？部署时要注意什么？

---

## 参考与延伸

- 下一章：[11.1 并发 bug 的特征](../ch11-testing-debugging/01-bug-characteristics.md)
- 上一节：[10.4 适合并行的算法](04-suitable-algorithms.md)
- 回到：[第 10 章](README.md)
