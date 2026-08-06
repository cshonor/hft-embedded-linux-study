# 10.1 三种执行策略

> 第 10 章 并行算法函数 · 上一章：[9.5 协程简介](../ch09-advanced-thread-management/05-coroutine.md) · 下一节：[10.2 par_unseq 的严格约束](02-par-unseq.md)

## 这节讲什么

C++17 给 `<algorithm>` 里的算法加了执行策略参数——`seq`/`par`/`par_unseq`——让它们自动并行化或向量化。本节讲三种策略的区别、使用方式、以及各策略的线程安全约束。

---

## 核心规则（代码+表格）

### 三种执行策略

```cpp
#include <execution>
#include <algorithm>

std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

// 1. 顺序执行（等价于传统算法）
std::sort(std::execution::seq, v.begin(), v.end());

// 2. 并行执行：可多线程，但单元素处理由单线程完成
std::for_each(std::execution::par, v.begin(), v.end(), [](int& x){
    x *= 2;
});

// 3. 并行 + 向量化：可多线程 + 单线程内 SIMD
std::for_each(std::execution::par_unseq, v.begin(), v.end(), [](int& x){
    x *= 2;
});
```

### 三种策略对比

| 策略 | 多线程 | 向量化(SIMD) | 元素间约束 |
|------|--------|-------------|-----------|
| `seq` | 否 | 否 | 严格顺序，可用 mutex 共享 |
| `par` | 是 | 否 | 元素可并行处理，单元素不需线程安全 |
| `par_unseq` | 是 | 是 | **最严**：不能用锁、不能阻塞 |

### 各策略的约束详解

```cpp
// seq：可以随便用锁
std::for_each(std::execution::seq, v.begin(), v.end(), [&mutex](int& x){
    std::lock_guard<std::mutex> lk(mutex);  // OK
    shared_result += x;
});

// par：可以用锁，但要小心性能
std::for_each(std::execution::par, v.begin(), v.end(), [&mutex](int& x){
    std::lock_guard<std::mutex> lk(mutex);  // 合法但竞争严重
    shared_result += x;
});
// 更好：用 reduction 或 atomic

// par_unseq：绝对不能用锁！
std::for_each(std::execution::par_unseq, v.begin(), v.end(), [&mutex](int& x){
    std::lock_guard<std::mutex> lk(mutex);  // UB！
    // SIMD 一个 lane 拿锁，其他 lane 死等 → 死锁
});
```

### 常见并行算法

```cpp
// 并行排序
std::sort(std::execution::par, v.begin(), v.end());

// 并行累加（返回值，不是原地）
int sum = std::reduce(std::execution::par, v.begin(), v.end(), 0);
// reduce 比 accumulate 更适合并行（无序结合）

// 并行查找
auto it = std::find(std::execution::par, v.begin(), v.end(), 42);

// 并行变换
std::transform(std::execution::par, v.begin(), v.end(), out.begin(),
               [](int x){ return x * x; });

// 并行填充
std::fill(std::execution::par, v.begin(), v.end(), 0);
```

### `reduce` vs `accumulate`

```cpp
// accumulate：严格从左到右，不能并行
int s = std::accumulate(v.begin(), v.end(), 0);  // seq only

// reduce：可无序结合（要求 op 满足结合律），可并行
int s = std::reduce(std::execution::par, v.begin(), v.end(), 0);
// 要求：op(op(a,b),c) == op(a,op(b,c))（结合律）
// 加法满足，但减法不满足！
```

---

## 新手要点（和 C 的区别）

- **C 没有标准并行算法**：C 程序员要并行化排序/累加，要么手写分块+线程，要么用 OpenMP（`#pragma omp parallel for`）。C++17 的并行 STL 让一行代码就能并行化——这是 C++ 相比 C 的巨大优势。
- **`#pragma omp` vs `execution::par`**：OpenMP 是编译指令，C++17 是库接口。OpenMP 更老更成熟，但需要编译器支持（`-fopenmp`）。C++17 并行算法是标准的，但实现依赖后端（MSVC 用 PPL，GCC 可选 TBB）。
- **`reduce` 要求结合律**：C 程序员可能习惯 `accumulate`（从左到右），但并行时顺序不确定——必须用 `reduce` 且操作满足结合律。减法、浮点加法（精度）要小心。
- **执行策略是编译期常量**：`std::execution::par` 不是运行时变量——不能 `if (use_parallel) policy = par;`。要运行时选择，得用 `if` 分支分别调用。

---

## HFT 关联

- **HFT 热路径不用并行 STL**：并行 STL 有任务调度开销（线程池、分块），不适合纳秒级热路径。HFT 热路径用固定流水线。
- **盘后批处理用并行 STL**：回测、因子计算、批量风控——`std::reduce(par, ...)` 比手写分块简洁且高效。一行代码从串行变并行。
- **`par_unseq` 的向量化**：批量数据处理（如因子计算）用 `par_unseq` 让编译器自动 SIMD——但回调不能有锁、不能有分支（否则向量化失败）。HFT 盘后可以用。
- **实现依赖**：HFT 系统如果在 Linux + GCC，并行 STL 后端通常是 TBB——要链接 `-ltbb`。MSVC 则自带 PPL 后端。部署时要注意。

---

## 自测题

1. `seq`、`par`、`par_unseq` 三种执行策略有什么区别？
2. 为什么 `par_unseq` 不能在回调中使用锁？
3. `std::reduce` 和 `std::accumulate` 有什么区别？为什么并行要用 `reduce`？
4. `reduce` 对操作有什么要求？减法可以用 `reduce` 吗？
5. 为什么 HFT 热路径不用并行 STL？什么 HFT 场景适合用？

---

## 参考与延伸

- 下一节：[10.2 par_unseq 的严格约束](02-par-unseq.md)
- 上一章：[9.5 协程简介](../ch09-advanced-thread-management/05-coroutine.md)
- 回到：[第 10 章](README.md)
