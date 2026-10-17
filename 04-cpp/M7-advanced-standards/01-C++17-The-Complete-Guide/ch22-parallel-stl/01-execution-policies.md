# 执行策略详解

## 三种执行策略

```cpp
#include <execution>
namespace ex = std::execution;

// seq：单线程顺序执行
std::for_each(ex::seq, v.begin(), v.end(), f);

// par：多线程并行
std::for_each(ex::par, v.begin(), v.end(), f);

// par_unseq：多线程 + 向量化（SIMD）
std::for_each(ex::par_unseq, v.begin(), v.end(), f);
```

## 策略对比

| 策略 | 线程 | 向量化 | 函数调用顺序 | 线程安全要求 |
|------|------|--------|-------------|-------------|
| `seq` | 单线程 | 无 | 严格顺序 | 无 |
| `par` | 多线程 | 无 | 不保证顺序 | 函数对象必须线程安全 |
| `par_unseq` | 多线程 | 有 | 不保证顺序 | 必须 lock-free、无数据依赖 |

## par_unseq 的额外限制

```cpp
// ❌ par_unseq 不能用 mutex——向量化中无法获取/释放锁
std::for_each(ex::par_unseq, v.begin(), v.end(), [&](int x) {
    std::lock_guard<std::mutex> lk(mtx);  // UB！可能死锁
    // ...
});

// ❌ 不能有跨元素数据依赖
std::for_each(ex::par_unseq, v.begin(), v.end(), [&](int x) {
    counter.fetch_add(1, std::memory_order_relaxed);  // 虽然合法，但序列化了
});

// ✅ 纯函数、无副作用
std::transform(ex::par_unseq, v.begin(), v.end(), out.begin(),
               [](int x) { return x * 2; });
```

## 执行策略对象

```cpp
// 三种策略是全局常量对象
constexpr std::execution::sequenced_policy seq{};
constexpr std::execution::parallel_policy par{};
constexpr std::execution::parallel_unsequenced_policy par_unseq{};

// 可传递策略参数
template <typename Policy>
void process(std::vector<int>& v, Policy pol) {
    std::sort(pol, v.begin(), v.end());
}

process(v, ex::par);       // 并行排序
process(v, ex::seq);       // 顺序排序
```

## 异常安全

```cpp
try {
    std::for_each(ex::par, v.begin(), v.end(), [](int x) {
        if (x < 0) throw std::runtime_error("negative");
    });
} catch (const std::exception& e) {
    // 并行算法中的异常：如果有线程抛异常，算法调用 std::terminate
    // 或由实现定义——不能假设异常被传播
}
```

**关键**：并行算法的异常语义——如果函数对象抛异常，算法可能调用 `std::terminate`，不保证异常传播。因此并行算法中的函数对象应避免抛异常。

## 性能实践

```cpp
// 大数据 + 简单操作 → 并行收益大
std::vector<double> big(10'000'000);
std::for_each(ex::par, big.begin(), big.end(), [](double& x) { x = std::sqrt(x); });

// 小数据 → 并行更慢（线程启动开销）
std::vector<int> small(100);
std::sort(ex::par, small.begin(), small.end());  // 可能比 seq 更慢
```

## 自测题

1. 三种执行策略的并行/向量化能力分别是什么？
2. `par_unseq` 为什么不能用 mutex？
3. 并行算法中函数对象抛异常会怎样？
4. 执行策略是类型还是对象？能不能作为函数参数传递？
5. 小数据为什么用 `par` 反而更慢？决定并行收益的关键因素是什么？

<details>
<summary>参考答案</summary>

1. `sequenced_policy`（`std::execution::seq`）：**不并行、不向量化**，在调用线程上顺序执行，语义与普通算法完全一致。
`parallel_policy`（`par`）：**多线程并行**，每个元素的访问由一个线程完成，但同一线程内元素访问不会交错。
`parallel_unsequenced_policy`（`par_unseq`）：**并行 + 向量化/无顺序**，同一线程内多个元素的执行可以交错（可 SIMD 化），不保证顺序。
2. `par_unseq` 允许同一线程内多个元素的执行**交错**（unsequenced）：若某个元素拿到 mutex 后要等待另一个元素释放，而后者因为交错无法推进，就会**死锁**；锁在向量化语境里也无法正确表达。
标准要求：`par_unseq` 下的元素访问函数不得获取任何锁，不得执行依赖其他元素访问的操作，必须无跨元素数据依赖、无副作用，也不能调用向量化不安全的库函数。
3. 按 C++17 标准，带 `par` / `par_unseq` 策略的算法，若其元素访问函数以**未捕获的异常**退出，属于**未定义行为**（实践中主流实现直接调用 `std::terminate`）。
所以不能假定异常会传播到调用方；正确做法是在 lambda 内部自己 `try/catch` 收集错误，或者改用 `seq`（`seq` 下异常按普通算法规则传播）。
4. 执行策略是**类型**（`std::execution::sequenced_policy`、`parallel_policy`、`parallel_unsequenced_policy`；C++20 另加 `unsequenced_policy`），而 `std::execution::seq` / `par` / `par_unseq` 是这些类型的**对象**（标准规定以内联常量或实现定义方式提供）。
可以作为函数参数传递——按值传参，或作为模板参数推导：
```cpp
template <typename Policy>
void process(std::vector<int>& v, Policy pol) {
    std::sort(pol, v.begin(), v.end());
}
process(v, std::execution::par);
```
用户不应自定义执行策略类型（该名字空间为标准保留）。
5. 小数据上并行要先做区间划分、任务调度、线程唤醒与同步，这些固定开销远大于几个元素的实际计算量，所以 `par` 反而更慢。
决定并行收益的关键因素是：**数据规模 n**、**每元素操作的计算强度**、**内存带宽与 cache 局部性**、**可用核数**，以及**实现质量**（线程池实现、是否真的开了并行，例如 libstdc++ 需要链接 TBB）。判断依据只有一条：实测，别凭直觉。

</details>
