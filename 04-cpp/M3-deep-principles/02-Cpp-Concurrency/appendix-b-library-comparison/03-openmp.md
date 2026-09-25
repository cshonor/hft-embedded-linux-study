# B.3 OpenMP 的定位

> 附录 B · 上一节：[B.2 C++ 标准库 vs TBB](02-std-vs-tbb.md) · 下一节：[B.4 ASIO 与网络并发](04-asio.md)

## 这节讲什么

OpenMP 用编译指令（`#pragma`）一行代码并行化循环——极其简洁，但灵活性有限。本节讲 OpenMP 的用法、与 C++ 并行 STL 的对比、以及它的适用边界。

---

## 核心规则（代码+表格）

### OpenMP 基础

```cpp
#include <omp.h>

// 并行 for：一行指令
#pragma omp parallel for
for (int i = 0; i < N; ++i) {
    data[i] *= 2;
}

// 指定线程数
#pragma omp parallel for num_threads(4)
for (int i = 0; i < N; ++i) {
    work(i);
}

// 归约（reduction）
int sum = 0;
#pragma omp parallel for reduction(+:sum)
for (int i = 0; i < N; ++i) {
    sum += data[i];
}
// OpenMP 自动处理每线程局部 + 合并
```

### 调度策略

```cpp
// static：均匀分块（默认）
#pragma omp parallel for schedule(static)
for (int i = 0; i < N; ++i) work(i);

// dynamic：动态分配（负载不均时）
#pragma omp parallel for schedule(dynamic, chunk_size)
for (int i = 0; i < N; ++i) work(i);  // 各 i 耗时不同

// guided：递减分块
#pragma omp parallel for schedule(guided)
for (int i = 0; i < N; ++i) work(i);
```

| 调度 | 分块 | 适用 |
|------|------|------|
| static | 固定大小，编译期确定 | 各迭代耗时相同 |
| dynamic | 固定 chunk，运行时分配 | 各迭代耗时不同 |
| guided | 递减 chunk | 负载不均 + 减少分配开销 |

### OpenMP vs C++ 并行 STL

```cpp
// OpenMP
#pragma omp parallel for reduction(+:sum)
for (int i = 0; i < N; ++i) sum += data[i] * data[i];

// C++17 并行 STL
int sum = std::transform_reduce(
    std::execution::par, data.begin(), data.end(), 0,
    std::plus<>(), [](int x){ return x * x; });
```

| 维度 | OpenMP | C++ 并行 STL |
|------|--------|-------------|
| 语法 | 编译指令 `#pragma` | 库调用 |
| 依赖 | 需编译器支持 `-fopenmp` | 需 TBB 后端（GCC） |
| 灵活性 | 低（只对循环） | 高（任意迭代器范围） |
| 调度控制 | `schedule()` 可控 | 实现内部决定 |
| 跨平台 | GCC/Clang/MSVC/Intel | 标准（但后端依赖不同） |
| 可读性 | 简洁（一行指令） | 需了解 STL 接口 |

### OpenMP 的优势场景

```cpp
// 1. 遗留 C 代码：OpenMP 不需要改写代码结构
// 原始 C 代码：
for (int i = 0; i < N; ++i) c[i] = a[i] + b[i];
// 加一行就能并行：
#pragma omp parallel for
for (int i = 0; i < N; ++i) c[i] = a[i] + b[i];

// 2. 科学计算：矩阵运算天然适合 OpenMP
#pragma omp parallel for collapse(2)  // 嵌套循环并行化
for (int i = 0; i < rows; ++i)
    for (int j = 0; j < cols; ++j)
        matrix[i][j] = compute(i, j);

// 3. 快速原型：不确定要不要并行，先加 pragma 试试
```

### OpenMP 的局限

```cpp
// 1. 只对循环有效
// 不能对树遍历、递归等结构并行化
// → 这些场景要用 TBB 或手写

// 2. 不支持复杂任务依赖（OpenMP 4.0+ 有 task depend，但不如 TBB Flow Graph）

// 3. 调试困难：pragma 不生成可见代码

// 4. 线程数全局影响：omp_set_num_threads() 影响所有并行区域
```

---

## 新手要点（和 C 的区别）

- **OpenMP 是 C/C++ 共用的**：C 程序员可能已经熟悉 OpenMP——它是 C 并行编程的主力工具。C++ 程序员可以选择 OpenMP 或并行 STL，但 C 程序员只有 OpenMP（或手写 pthread）。
- **`#pragma` 是编译指令不是代码**：C 程序员如果用过 OpenMP，知道 `#pragma` 在不支持 OpenMP 的编译器上会被忽略（变成串行）——这是优点（可移植）也是陷阱（以为并行了其实没有）。
- **C++ 并行 STL 更"C++风格"**：C 程序员转型 C++ 后，可能仍习惯 OpenMP——但 C++17 并行 STL 用迭代器，和 STL 算法风格一致，更符合 C++ 习惯。
- **OpenMP 的 `collapse` 很强大**：嵌套循环并行化用 `collapse(2)`——C++ 并行 STL 做这个更麻烦（要手动展平或用 `for_each` + 内层循环）。

---

## HFT 关联

- **HFT 热路径不用 OpenMP**：OpenMP 有线程池开销和调度不确定性——不适合纳秒级热路径。
- **盘后科学计算用 OpenMP**：如果 HFT 团队有 C 科学计算背景，盘后分析（如统计模型、蒙特卡洛）用 OpenMP 很自然——比改写成 C++ STL 代码量少。
- **`schedule(dynamic)` 用于负载不均**：盘后因子计算如果各标的耗时差异大，用 `schedule(dynamic)` 自动均衡——类似 work-stealing 的效果。
- **OpenMP 和 TBB 的选择**：HFT 盘后工具如果已经在用 OpenMP（C 遗留代码），不必强制迁移到 TBB——两者性能相近，迁移成本不值得。新代码可以用 C++ 并行 STL。

---

## 自测题

1. OpenMP 和 C++17 并行 STL 在语法上有什么区别？
2. `schedule(static)`、`schedule(dynamic)`、`schedule(guided)` 各适合什么场景？
3. OpenMP 的 `reduction(+:sum)` 做了什么？
4. OpenMP 有什么局限？什么场景不适合用 OpenMP？
5. HFT 盘后分析用 OpenMP 还是 C++ 并行 STL？如何选择？

<details>
<summary>参考答案</summary>

1. OpenMP 通过 `#pragma omp ...` 标注循环或并行区域，由编译器和 runtime 处理线程与任务。
   并行 STL 在标准算法调用中传入 `std::execution` 策略，代码更贴近库算法但只覆盖可表达为标准算法的工作。
2. `static` 预先固定分块，开销低，适合每次迭代成本接近；`dynamic` 运行时领取小块，适合高度不均匀任务。
   `guided` 从大块逐步减小，在负载均衡和调度开销间折中。
3. `reduction(+:sum)` 为各线程建立私有的局部累加值，区域结束时再按 `+` 合并。
   它避免每次迭代争用同一变量；浮点合并顺序可能变化，所以结果不保证逐位等同串行。
4. OpenMP 依赖编译器和 runtime，线程调度与嵌套行为较隐式，也不擅长复杂异步控制流和严格尾延迟场景。
   小循环、I/O 密集任务、需精确绑核/内存布局或低延迟热路径通常不适合。
5. 规则数值循环、已有循环代码且需要 reduction/schedule 控制时可优先 OpenMP。
   能自然表示为标准算法且重视标准接口与可组合性时可用并行 STL；最终按工具链支持和基准结果选择。

</details>

---

## 参考与延伸

- 下一节：[B.4 ASIO 与网络并发](04-asio.md)
- 上一节：[B.2 C++ 标准库 vs TBB](02-std-vs-tbb.md)
- 回到：[附录 B](README.md)
