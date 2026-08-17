# 10.2 par_unseq 的严格约束

> 第 10 章 · 上一节：[10.1 三种执行策略](01-execution-policies.md) · 下一节：[10.3 并行算法的复杂度保证](03-complexity.md)

## 这节讲什么

`par_unseq` 允许编译器向量化（SIMD）+ 多线程并行——性能最强，但约束也最严。本节详解为什么 `par_unseq` 下不能用锁、不能用 `atomic` 的阻塞操作、以及如何写"向量化友好"的回调。

---

## 核心规则（代码+表格）

### 为什么 `par_unseq` 不能用锁

```cpp
// SIMD 向量化的本质：一个线程同时处理多个元素
// 例如 AVX2：一次处理 8 个 int

// 假设回调是：
[](int& x) {
    std::lock_guard<std::mutex> lk(m);  // 锁
    shared += x;
}

// 向量化后变成（伪代码）：
// 8 个 lane 同时执行：
//   lane0: lock(m); shared += x[0];
//   lane1: lock(m); shared += x[1];  // ← 等 lane0 释放
//   lane2: lock(m); shared += x[2];  // ← 等 lane1
//   ...
// 但 lock(m) 是串行的——8 个 lane 争同一把锁
// 而且 lock 可能调用系统调用（futex）—— 向量化代码不能有系统调用
// → 未定义行为（UB）
```

### `par_unseq` 的禁止操作

| 禁止操作 | 原因 |
|----------|------|
| `mutex::lock()` | 系统调用 + 串行化，破坏向量化 |
| `condition_variable::wait()` | 阻塞，可能永远不被唤醒 |
| `atomic::wait()` / `notify()` | 阻塞操作 |
| 复杂控制流（switch/异常） | 阻止编译器向量化 |
| 函数调用（非 inline） | 可能破坏向量化（取决于编译器） |

### 允许的操作

| 允许操作 | 说明 |
|----------|------|
| 纯计算 | `x = x * 2 + 1` |
| `atomic` 的非阻塞操作 | `fetch_add`（relaxed）可以，但要注意 cache bounce |
| 只读共享数据 | 多个 lane 同时读无竞争 |
| 写不同位置 | `out[i] = f(in[i])` |

### 向量化友好的回调

```cpp
// 好：纯计算，无锁、无共享
std::for_each(std::execution::par_unseq, v.begin(), v.end(), [](int& x){
    x = x * 2 + 1;  // 纯计算
});

// 好：写入不同位置
std::transform(std::execution::par_unseq, in.begin(), in.end(), out.begin(),
               [](int x){ return x * x; });

// 差：共享变量 + atomic（合法但性能差）
std::for_each(std::execution::par_unseq, v.begin(), v.end(), [&sum](int x){
    sum.fetch_add(x, std::memory_order_relaxed);  // 合法但 cache bounce
});
// 更好：用 reduce
int s = std::reduce(std::execution::par_unseq, v.begin(), v.end(), 0);

// UB：用锁
std::for_each(std::execution::par_unseq, v.begin(), v.end(), [&m, &sum](int x){
    std::lock_guard<std::mutex> lk(m);  // UB！
    sum += x;
});
```

### 向量化失败的常见原因

```cpp
// 1. 分支太多
[](int& x){
    if (x > 0) x = 1;     // 分支
    else if (x < 0) x = -1;
    else x = 0;
};
// 更好：用三元或位运算（无分支）
[](int& x){
    x = (x > 0) - (x < 0);  // 无分支
};

// 2. 函数调用
[](int& x){
    x = expensive_func(x);  // 如果不 inline，向量化失败
};
// 更好：标记 inline 或用仿函数

// 3. 数据依赖
for (int i = 1; i < n; ++i)
    v[i] += v[i-1];  // 前缀和，有依赖，不能向量化
// 用 std::exclusive_scan(par_unseq, ...) 替代
```

---

## 新手要点（和 C 的区别）

- **C 程序员通常手动写 SIMD**：用 `__m256i`（AVX2）或 intrinsics 手动向量化——精确但难写。C++17 的 `par_unseq` 让编译器自动向量化——写普通代码，编译器优化。但回调必须"向量化友好"。
- **"向量化友好"是新概念**：C 程序员写普通代码可能不会考虑"这段代码能不能被 SIMD"——但用 `par_unseq` 时必须考虑。分支、函数调用、锁都会阻止向量化。
- **无分支编程**：C 程序员可能习惯 `if-else`——但在 `par_unseq` 下，分支会破坏向量化。用三元运算符或位运算替代（编译器能生成 `cmov` 指令）。
- **`atomic` 在 `par_unseq` 下要小心**：C 程序员可能觉得"atomic 总是安全的"——但在 `par_unseq` 下，`atomic::wait/notify` 是 UB（阻塞），`fetch_add` 合法但性能差（cache bounce）。最好用 `reduce` 等专用并行算法。

---

## HFT 关联

- **HFT 热路径手动 SIMD 而非 `par_unseq`**：HFT 对性能要求极致——手动写 AVX2/AVX-512 intrinsics 比 `par_unseq` 更可控。`par_unseq` 用于盘后批处理。
- **因子计算用 `par_unseq`**：批量计算因子（如全市场 RSI、MACD）用 `par_unseq` + 纯计算回调——编译器自动 SIMD，性能接近手写。
- **`reduce(par_unseq, ...)` 替代手写并行累加**：盘后统计（如总成交量、最大涨跌幅）用 `reduce(par_unseq, ...)`——一行代码，编译器优化。
- **向量化友好的代码风格**：HFT 的热路径代码也应遵循"向量化友好"原则——无分支、无函数调用、数据连续。即使不用 `par_unseq`，这种风格也有利于编译器自动向量化。

---

## 自测题

1. 为什么 `par_unseq` 下不能用 `mutex::lock()`？SIMD lane 之间会发生什么？
2. `par_unseq` 下 `atomic::fetch_add` 合法吗？有什么性能问题？
3. 什么样的回调是"向量化友好"的？举一个好例子和一个坏例子。
4. 为什么 `if-else` 分支会破坏向量化？如何改写？
5. HFT 热路径用 `par_unseq` 还是手动 SIMD？为什么？

---

## 参考与延伸

- 下一节：[10.3 并行算法的复杂度保证](03-complexity.md)
- 上一节：[10.1 三种执行策略](01-execution-policies.md)
- 回到：[第 10 章](README.md)
