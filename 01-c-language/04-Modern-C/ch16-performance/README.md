# Ch16 · Performance（性能） ④🔴

> **Level 3 · 深入** · 策略：**🔴 精读** · 阅读顺序 ④
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

> **第 4 本书 · Ch16** · 全书对 HFT 最实用的一章——`restrict` 是零成本优化提示，memcpy/热路径函数必用。

## 本章讲什么

先正确再快、`inline` 函数、**`restrict` 限定符（别名消除）**、测量与 profiling。

## 小节索引

| 节 | 标题 | 核心知识点 |
|----|------|------------|
| [16.1](./16.1-内联函数.md) | 内联函数 | `static inline` 消除调用开销；`[[gnu::always_inline]]` 强制内联 |
| [16.2](./16.2-restrict限定符.md) | restrict 限定符 | **重点**：别名消除；零成本向量化；memcpy vs memmove |
| [16.3](./16.3-测量和检验.md) | 测量和检验 | 先正确再快；`-O2`/`-O3` 优化选项；perf profiling 流程 |

## HFT / DPDK 关联总结

| 概念 | HFT 应用 |
|------|----------|
| **`restrict`** | 热路径函数别名消除，零成本优化，memcpy/处理函数必用 |
| **`static inline`** | 头文件内联函数，消除调用开销 |
| **`[[gnu::always_inline]]`** | 强制内联关键路径函数 |
| **`-O3`** | 向量化 + 内联 + 循环展开 |
| **profiling** | `perf` 找热点，只优化 profiler 证明的瓶颈 |
| **先正确再快** | 算法/数据布局优先于微优化 |

## 自测题

<details><summary>1. <code>restrict</code> 对编译器优化有什么具体影响？</summary>

restrict 承诺指针不与其它指针别名同一内存。编译器可以：① 缓存读取值（不必每次重新读，因为不会被其它写改变）；
② 重排读写顺序（没有依赖关系）；③ 向量化循环（批量读、批量写，SIMD）。这些优化在指针可能别名时
编译器不敢做。restrict 是零成本优化——不生成任何运行时检查代码，只是给编译器的编译期承诺。
</details>

<details><summary>2. <code>memcpy</code> 和 <code>memmove</code> 有什么区别？为什么？</summary>

`memcpy` 的参数有 `restrict`：假设源和目标不重叠，可以按最快方式复制（正向批量拷贝）。
`memmove` 没有 `restrict`：允许重叠，必须先检查方向（如果目标在源前面则正向拷贝，否则反向拷贝）。
如果源和目标重叠时调用 `memcpy`，是 UB。HFT 中明确不重叠的场景优先用 `memcpy`。
</details>

<details><summary>3. 什么时候不该加 <code>restrict</code>？</summary>

当两个指针可能指向同一内存（或重叠区域）时不加 restrict。例如：
① `add_arrays(arr, arr, n)` — 同一数组传两次参数；
② 处理可能重叠的内存区域；
③ 不确定调用者是否会传别名指针的通用库函数。
加了 restrict 后违反承诺是 UB——比不加更危险，因为编译器优化后可能产生完全错误的结果。
</details>

<details><summary>4. 为什么"先正确再快"是 HFT 的黄金法则？</summary>

HFT 系统的正确性要求极高——一个 bug 可能导致错误交易、巨额亏损。过早优化会：
① 增加代码复杂度，隐藏 bug；② 调试困难（优化的代码难以单步跟踪）；③ 浪费时间优化非热点。
正确流程：先写清晰正确的代码 → profile 找热点 → 只优化 profiler 证明的瓶颈。90% 的性能来自
算法和数据布局，微优化（restrict/inline）只影响最后 10%。
</details>

<details><summary>5. <code>-O3</code> 比 <code>-O2</code> 做了什么额外优化？有风险吗？</summary>

`-O3` 额外做：激进内联、循环向量化（AVX2/AVX-512）、循环展开、循环交换。
风险：① 代码体积增大（指令缓存压力）；② 某些情况下可能变慢（内联过度导致 icache miss）；
③ `-Ofast` 会开启 `-ffast-math`，打破 IEEE 754 浮点语义（HFT 计算中可能导致精度问题）。
建议：HFT 用 `-O3` 但不用 `-Ofast`，浮点计算需要严格 IEEE 754 语义。
</details>
