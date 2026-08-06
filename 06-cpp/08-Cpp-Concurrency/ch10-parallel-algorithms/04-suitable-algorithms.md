# 10.4 适合并行的算法

> 第 10 章 · 上一节：[10.3 并行算法的复杂度保证](03-complexity.md) · 下一节：[10.5 与传统手写并行的对比](05-vs-manual.md)

## 这节讲什么

不是所有算法都适合并行化。本节讲哪些算法天然并行友好（`reduce`、`transform`、`fill`）、哪些有数据依赖不适合（前缀和、迭代式算法）、以及如何选择或改写算法以获得并行收益。

---

## 核心规则（代码+表格）

### 并行友好 vs 不友好的算法

| 算法 | 并行友好？ | 原因 |
|------|-----------|------|
| `reduce` / `transform_reduce` | 是 | 元素独立，满足结合律 |
| `transform` | 是 | 元素独立写入 |
| `fill` / `generate` | 是 | 元素独立 |
| `copy` | 是 | 元素独立 |
| `sort` | 是（有专门实现） | 分治可并行 |
| `find` / `any_of` / `all_of` | 部分 | 提前终止与并行冲突 |
| `accumulate` | 否 | 严格从左到右，有依赖 |
| `partial_sum` / `exclusive_scan` | 部分 | 有前缀依赖，但可分治 |
| 迭代式算法（牛顿法） | 否 | 每步依赖上一步 |

### `transform_reduce`：并行王者

```cpp
// transform_reduce = transform + reduce，两步合一
// 适合"先变换再归约"的场景

// 例：计算向量各元素的平方和
std::vector<int> v = {1, 2, 3, 4, 5};

// 串行：
int sum_sq = std::accumulate(v.begin(), v.end(), 0,
    [](int acc, int x){ return acc + x*x; });

// 并行：
int sum_sq = std::transform_reduce(
    std::execution::par,
    v.begin(), v.end(),
    0,                          // 初始值
    std::plus<int>(),           // reduce 操作（结合律）
    [](int x){ return x*x; }    // transform 操作
);
// 元素先各自平方，再并行归约——完全并行友好
```

### 前缀和的并行化

```cpp
// 串行前缀和：O(N)，严格依赖
std::partial_sum(v.begin(), v.end(), out.begin());
// out[i] = v[0] + v[1] + ... + v[i]
// 每个元素依赖前面所有元素 → 看似不能并行

// 并行前缀和：O(N log N) 但可并行
std::exclusive_scan(std::execution::par, v.begin(), v.end(), out.begin(), 0);
// 算法：分块计算各块和 → 用块和做前缀 → 块内前缀和
// 虽然操作次数更多（O(N log N) vs O(N)），但可并行 → 实际更快
```

### 不适合并行的模式

```cpp
// 1. 迭代依赖
for (size_t i = 1; i < v.size(); ++i)
    v[i] += v[i-1];  // 每个依赖前一个 → 不能并行

// 2. 状态累积
int state = 0;
for (auto& x : v) {
    state = update(state, x);  // state 有依赖 → 不能并行
    x = state;
}

// 3. 动态条件
for (auto& x : v) {
    if (some_global_flag) break;  // 全局状态影响控制流 → 难并行
    process(x);
}
```

### 改写为并行友好

```cpp
// 不友好：带状态的循环
double running_avg = 0;
for (size_t i = 0; i < v.size(); ++i) {
    running_avg = (running_avg * i + v[i]) / (i + 1);
    out[i] = running_avg;
}

// 改写：先并行求和，再并行算前缀
// 1. 并行求总和
double total = std::reduce(std::execution::par, v.begin(), v.end(), 0.0);
// 2. 并行前缀和
std::exclusive_scan(std::execution::par, v.begin(), v.end(), prefix.begin(), 0.0);
// 3. 并行计算 running avg
std::transform(std::execution::par, v.begin(), v.end(), out.begin(),
    [&](double x, size_t i){ return prefix[i+1] / (i+1); });
// 步骤更多，但每步可并行 → 大数据量时更快
```

---

## 新手要点（和 C 的区别）

- **C 程序员可能不知道"并行友好"的概念**：C 里写循环通常不考虑"能不能并行"——因为 C 没有标准并行算法。C++17 的并行 STL 要求程序员理解哪些算法适合并行。
- **`transform_reduce` 是 C++17 新增**：C 程序员可能没见过——它把"变换+归约"合成一步，完全并行友好。这是 C++ 并行编程的利器。
- **"操作次数更多但更快"是反直觉的**：C 程序员可能觉得 O(N log N) 比 O(N) 慢——但在并行场景下，O(N log N) 的并行版可能比 O(N) 的串行版快（因为多核并行）。这是并行思维和串行思维的核心差异。
- **改写算法需要技巧**：C 程序员可能习惯"带状态的循环"——但在并行场景下，要拆解成无状态的并行步骤。这需要算法设计能力，不是简单加 `par` 就行。

---

## HFT 关联

- **`transform_reduce` 用于因子计算**：批量计算因子（如全市场方差 = `transform_reduce(par, begin, end, 0, plus, square)`）——一行代码，完全并行。
- **前缀和用于累积收益**：计算策略累积收益曲线用 `exclusive_scan(par, ...)`——虽然操作次数比串行多，但大数据量下并行更快。
- **改写"带状态循环"**：HFT 盘后分析中如果有"逐 tick 更新状态"的循环，考虑能否拆解成并行步骤——如先并行分段求和，再合并。
- **迭代式算法不并行**：HFT 的参数校准（如 Newton-Raphson 优化）每步依赖上一步——不能并行。但可以并行校准多个参数（任务并行）。

---

## 自测题

1. `reduce`/`transform`/`fill` 为什么是"并行友好"的？
2. `accumulate` 为什么不能并行？用什么替代？
3. `partial_sum` 看似有前缀依赖，如何并行化？为什么操作次数更多反而更快？
4. `transform_reduce` 解决了什么问题？举一个使用场景。
5. "带状态的循环"如何改写为并行友好？

---

## 参考与延伸

- 下一节：[10.5 与传统手写并行的对比](05-vs-manual.md)
- 上一节：[10.3 并行算法的复杂度保证](03-complexity.md)
- 回到：[第 10 章](README.md)
