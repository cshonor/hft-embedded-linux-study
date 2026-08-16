# Ch16 · Performance（性能）

> **Level 3 · 深入** · 策略：**🔴 精读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

先正确再快、`inline` 函数、**`restrict` 限定符（别名消除）**、测量与 profiling。
**全书对 HFT 最实用的一章**——`restrict` 是零成本优化提示，memcpy/热路径函数必用。

## 一、Safety first（先正确再快）

### 黄金法则

> **Make it work, make it right, make it fast.** — Kent Beck

| 阶段 | 目标 | 方法 |
|------|------|------|
| 1. Work | 功能正确 | 写能跑的代码 |
| 2. Right | 代码清晰 | 重构、加测试、code review |
| 3. Fast | 性能优化 | **先 profile，再优化热点** |

```c
/* ❌ 过早优化：代码还没跑对就手写 SIMD */
/* ✅ 先写清晰的标量代码，profile 后只优化热点 */
```

> 与 [06.6 Systems Performance](../../06.6-systems-performance/) 的方法论呼应：**测量先于优化**。

### 优化级别

| 级别 | 手段 | 收益 | 风险 |
|------|------|------|------|
| 算法 | O(n²) → O(n log n) | 数量级 | 低 |
| 数据布局 | SoA vs AoS、对齐 | 2-10x | 中 |
| 编译器优化 | `-O2`/`-O3`、`restrict`、`inline` | 10-50% | 低 |
| 手写汇编/SIMD | AVX2/AVX-512 | 2-5x（特定场景） | 高（可移植性、维护性） |

> **HFT 原则**：算法和数据布局是第一优先级；编译器优化第二；手写汇编最后（只在 profiler 证明瓶颈后）。

## 二、`inline` 函数

### 为什么要 inline

| 普通函数调用 | inline 展开后 |
|-------------|--------------|
| 压栈/跳转/退栈 | 代码直接嵌入调用处 |
| 分支预测失败风险 | 无跳转，无分支预测 |
| 参数传递开销 | 参数直接用寄存器/常量 |
| ~3-5ns 延迟 | ~0ns |

### inline 的使用方式

```c
/* 头文件中：static inline（内核/DPDK 标配） */
static inline uint32_t ring_next_idx(uint32_t idx, uint32_t mask) {
    return (idx + 1) & mask;
}

/* 强制内联 */
[[gnu::always_inline]] static inline void hot_path_fn(void) { }

/* 禁止内联（调试用） */
[[gnu::noinline]] static void cold_path_fn(void) { }
```

### 编译器对 inline 的决策

| 因素 | 倾向 inline | 倾向不 inline |
|------|------------|--------------|
| 函数体大小 | 小（1-3 行） | 大（循环、递归） |
| 调用次数 | 多处调用 | 只调一次 |
| 是否在热路径 | 是 | 否 |
| `-O` 级别 | `-O2`/`-O3` | `-O0` |
| `[[gnu::always_inline]]` | 强制 | — |
| `[[gnu::noinline]]` | — | 强制 |

> **HFT 建议**：热路径小函数用 `static inline`；关键函数加 `[[gnu::always_inline]]` 确保内联。
> 详见 [Ch7 函数](../ch07-functions/README.md)。

## 三、`restrict` 限定符（别名消除 — 重点）

### 什么是别名（Aliasing）

**别名** = 两个指针指向同一块内存。编译器必须假设任何两个指针可能别名，因此不能自由优化。

```c
/* 没有 restrict：编译器必须假设 a 和 b 可能指向同一内存 */
void add_arrays(int *a, int *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        a[i] += b[i];   // 每次写 a[i] 后，b[i] 可能被改 → 必须重新读 b[i]
    }
}

/* 有 restrict：编译器知道 a 和 b 不别名 → 可以向量化、重排 */
void add_arrays(int *restrict a, const int *restrict b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        a[i] += b[i];   // b[i] 不会被 a[i] 的写影响 → 可以批量读、批量写
    }
}
```

### restrict 的语义

```c
/* restrict 承诺：在指针生命周期内，所有通过该指针访问的内存，
   只通过这一个指针访问（没有其它指针别名同一内存）。 */

void f(int *restrict p, int *restrict q) {
    *p = 1;
    *q = 2;   // 编译器知道：写 q 不会影响 p 指向的内存
}

int x;
f(&x, &x);   // ❌ UB：违反 restrict 承诺（p 和 q 别名同一内存）
```

| restrict 承诺 | 编译器获益 |
|---------------|-----------|
| 指针不与其它指针别名 | 可以缓存读取值，不必每次重新读 |
| 写不会影响其它指针指向的内存 | 可以重排读写顺序 |
| 没有副作用依赖 | 可以向量化（SIMD） |

### 标准库中的 restrict

```c
/* C99 起，标准库函数签名都加了 restrict */
void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *strcpy(char *restrict dest, const char *restrict src);
int  printf(const char *restrict format, ...);
int  sprintf(char *restrict s, const char *restrict format, ...);
```

> `memcpy` 和 `memmove` 的区别就在 restrict：`memcpy` 假设不重叠（可以快速复制），
> `memmove` 允许重叠（必须检查方向，较慢）。

### HFT 热路径中的 restrict

```c
/* HFT 报文处理函数加 restrict */
void process_orders(
    const struct order *restrict orders,   // 输入：不会被修改
    struct result *restrict results,        // 输出：不与输入别名
    size_t n
) {
    for (size_t i = 0; i < n; i++) {
        results[i].price = orders[i].price * orders[i].qty;
        results[i].id    = orders[i].id;
    }
    /* 编译器可以：① 向量化循环 ② 缓存 orders[i] 的值 ③ 重排读写 */
}
```

### restrict 的风险

```c
/* ❌ 违反 restrict 承诺 = UB */
int arr[10] = {0};
add_arrays(arr, arr, 10);   // a 和 b 别名同一数组 → UB

/* 正确做法：用 memmove 风格（允许重叠）或不加 restrict */
void add_arrays_safe(int *a, const int *b, size_t n) { ... }  // 没有 restrict
```

| 规则 | 说明 |
|------|------|
| 只在确定不别名时加 `restrict` | 违反承诺是 UB，比不加更危险 |
| 加了就要测试 | 开 `-O2` 后行为可能变化 |
| `restrict` 是编译期零成本 | 不产生任何运行时检查代码 |
| 可以加到结构体成员 | `struct { int *restrict p; }` |

> **HFT 建议**：热路径函数的输入/输出指针参数加 `restrict`。这是零成本的优化提示，
> 编译器可向量化循环、消除冗余读取。但必须确保调用时指针确实不别名。

### restrict 与 const 的配合

```c
/* 最佳实践：const + restrict */
void transform(
    const float *restrict input,    // 只读 + 不别名 → 编译器可预取
    float *restrict output,         // 只写 + 不别名 → 编译器可批量写
    size_t n
);
```

| 组合 | 含义 |
|------|------|
| `const T *restrict p` | 不通过 p 修改数据 + 不与其它指针别名 |
| `T *restrict p` | 可通过 p 修改数据 + 不与其它指针别名 |
| `const T *p`（无 restrict） | 不修改数据 + 可能与其它指针别名 |

## 四、测量与 profiling

### 编译器优化选项

| 选项 | 效果 | HFT 建议 |
|------|------|----------|
| `-O0` | 不优化（调试用） | 仅 debug 构建 |
| `-O1` | 基本优化 | — |
| `-O2` | 标准优化（大多数项目） | ✅ 生产默认 |
| `-O3` | 激进优化（向量化、内联） | ✅ HFT 热路径 |
| `-Os` | 优化大小 | — |
| `-Ofast` | `-O3` + `-ffast-math` | ⚠️ 打破 IEEE 754 |

### `-O3` 的关键优化

| 优化 | 说明 |
|------|------|
| 循环向量化 | 自动将标量循环转为 SIMD（AVX2/AVX-512） |
| 函数内联 | 更激进的内联（包括跨文件 LTO） |
| 循环展开 | 减少分支开销 |
| 自动并行化 | OpenMP 风格的多线程（需要 `-ftree-parallelize-loops`） |

### profiling 工具

| 工具 | 用途 | HFT 场景 |
|------|------|----------|
| `perf` | CPU profiling、cache miss、branch miss | 热路径分析 |
| `perf stat` | 高层统计（IPC、cache 命中率） | 快速评估 |
| `perf record` | 采样分析 | 找热点函数 |
| `valgrind --tool=cachegrind` | cache 模拟 | 伪共享检测 |
| `Intel VTune` | 商业级 profiler | 深度分析（cache、内存、线程） |
| `google-perftools` | 采样 profiler | 低开销 profiling |

```bash
# HFT 典型 profiling 流程
perf stat -e cycles,instructions,cache-misses,branch-misses ./hft_engine

perf record -g ./hft_engine
perf report
```

> 详见 [06.6 Systems Performance](../../06.6-systems-performance/) 的 perf 章节。

### 编译器优化报告

```bash
# gcc 优化报告
gcc -O3 -fopt-info-vec         # 哪些循环被向量化
gcc -O3 -fopt-info-inline      # 哪些函数被内联
gcc -O3 -fopt-info-missed      # 哪些优化没做及原因

# 查看生成的汇编
gcc -O3 -S hello.c             # 输出 hello.s
objdump -d hello               # 反汇编
```

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
