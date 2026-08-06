# 第 10 章 并行算法函数

**Parallel Algorithms**

## 本章讲什么

C++17 引入并行 STL——给 `<algorithm>` 里的算法加执行策略参数，让它们自动并行化。本章讲 `execution::par`/`par_unseq`/`seq` 三种策略、并行算法的正确性约束、以及与 C++20 `std::ranges` 的配合。

## 要点

### 三种执行策略

```cpp
#include <execution>
#include <algorithm>

std::vector<int> v = ...;

// 1. 顺序（等价于传统算法）
std::sort(std::execution::seq, v.begin(), v.end());

// 2. 并行：可多线程，但每个元素的处理由单线程完成
std::sort(std::execution::par, v.begin(), v.end());

// 3. 并行+向量化：可多线程 + 单线程内可向量化（SIMD）
std::for_each(std::execution::par_unseq, v.begin(), v.end(), [](int& x){
    x *= 2;
});
```

| 策略 | 并行 | 向量化 | 调用约束 |
|------|------|--------|----------|
| `seq` | 否 | 否 | 可用 mutex 保护的共享 |
| `par` | 是 | 否 | 元素访问可并行，但单元素操作不需线程安全 |
| `par_unseq` | 是 | 是 | **最严**：不能用锁、不能用 `atomic` 的阻塞操作 |

### `par_unseq` 的严格约束

`par_unseq` 允许编译器向量化，意味着同一线程可能"同时"处理多个元素（SIMD）。因此回调函数**不能**：
- 调用 mutex 的 `lock()`（一个 SIMD lane 拿锁，其他 lane 死等）
- 用可能阻塞的同步原语
- 有数据依赖的跨元素操作

可以用 `relaxed` 的 atomic（非阻塞），但任何可能阻塞的操作都违反契约。

### 并行算法的复杂度保证

C++ 标准对并行算法的复杂度要求：
- `std::sort(par)`：O(n log n)，但常数因子可能比串行大。
- `std::reduce(par)`：要求操作满足结合律（不要求交换律），因此 `float` 加法结果可能与串行不同。
- `std::transform_reduce(par)`：map + reduce，适合数据并行计算。

### 适合并行的算法

| 算法 | 并行友好度 | 说明 |
|------|-----------|------|
| `for_each` | 高 | 无依赖，天然并行 |
| `transform` | 高 | 逐元素映射 |
| `reduce` | 高 | 但需结合律 |
| `sort` | 中 | 并行归并/快排，常数大 |
| `find`/`any_of` | 中 | 找到即停，需原子标志 |
| `partition` | 中 | 数据搬运有竞争 |

### 与传统手写并行的对比

```cpp
// 手写（第 8 章风格）
std::vector<std::thread> ts;
size_t block = v.size() / N;
for (...) ts.emplace_back([...]{ /* 分块处理 */ });
for (auto& t : ts) t.join();

// C++17 并行算法
std::for_each(std::execution::par, v.begin(), v.end(), work);
```

并行算法省去了手动划分、join、异常处理的样板代码。但**底层实现质量决定性能**——MSVC、libstdc++、TBB 后端的表现差异很大，需实测。

## HFT 关联

- **并行 STL 在 HFT 热路径慎用**：并行算法有线程调度开销（启动/同步），数据量小（< 1 万元素）时比串行慢。HFT 单笔行情处理数据量小，用串行更快。
- **批量回测场景适用**：回测引擎对历史数据批量计算（夏普、回撤、统计）用 `transform_reduce(par)` 加速。
- **`par_unseq` + SIMD 的潜力**：信号计算（向量化指标）用 `par_unseq` 让编译器自动 SIMD，但回调必须无锁无阻塞。
- **`reduce` 的浮点陷阱**：并行 reduce 改变结合顺序，浮点结果与串行不同——回测结果复现要固定策略（用 `seq` 或固定分块）。
- **确定性优先于速度**：HFT 回测要求结果可复现，并行算法的非确定性（调度顺序）可能让结果波动——关键路径用 `seq`。

## 自测题

1. `execution::par` 和 `par_unseq` 的区别是什么？`par_unseq` 为什么不能用 mutex？
2. 为什么 `std::reduce(par)` 对 `float` 加法的结果可能与串行不同？
3. 哪些算法并行友好，哪些不友好？`find` 并行化有什么难点？
4. HFT 热路径为什么一般不用并行 STL？什么场景适合用？
5. 并行算法和手写分块多线程相比，优缺点是什么？
