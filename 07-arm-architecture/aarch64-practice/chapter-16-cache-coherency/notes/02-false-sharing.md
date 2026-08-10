# §16.2 伪共享（False Sharing）

> **来源：** [Ch16 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

不同核频繁写同一 cache line 的不同变量时，MESI 协议导致该行在核间反复搬运（invalidate → reload），性能暴跌。本节分析伪共享的产生机制、检测方法和修复手段，给出代码级修复方案和性能对比数据。

## 核心要点

### 伪共享机制

```c
// 两个变量在同一 Cache Line（64字节）内
struct {
    int a;    // offset 0，CPU0 频繁写
    int b;    // offset 4，CPU1 频繁写
    // padding... 共 64 字节
} data;
```

Cache line 在核间搬运的完整过程：

```
T0: CPU0 写 a → L1 D-cache 行加载（Exclusive 状态）
T1: CPU1 写 b → 发现该行在 CPU0 是 E/M 状态
    → 总线请求 → CPU0 发送 cache line 给 CPU1
    → CPU0 的行变 S（Shared），CPU1 的行变 S
T2: CPU0 再写 a → S 状态写需要 invalidate 其他核
    → 发送 Invalidate 消息 → CPU1 的行变 I
    → CPU0 的行变 M（Modified）
T3: CPU1 再写 b → 行是 I，需要重新加载
    → 从 CPU0 的 M 行获取（或从内存）→ CPU0 变 S → CPU1 变 S
    → 循环 T2-T3...
```

- 每次写操作都触发跨核 cache line 传输（~50-100ns/次）
- 如果写入频率高（如每秒百万次），累计延迟巨大
- 两个变量完全无关，却因物理位置共享 cache line 而互相干扰

### Cache Line 大小确认

```c
// 获取当前处理器的 cache line 大小
#include <unistd.h>
long cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
// ARM A72/A76 = 64 字节

// C11 标准 API
size_t clsize = 0;
pthread_getaffinity_np(/* ... */);
// 或硬编码（ARMv8 固定 64 字节）
#define CACHE_LINE_SIZE 64
```

### 修复方法

```c
// 方法1：手动 padding（最简单）
struct {
    int a;
    char pad[60];   // 填充到 64 字节
    int b;
} data;

// 方法2：GCC 属性对齐（推荐）
struct data {
    int a __attribute__((aligned(64)));
    int b __attribute__((aligned(64)));
};

// 方法3：C++11 alignas
struct alignas(64) PerCpuStats {
    uint64_t order_count;
    uint64_t latency_sum;
    // 编译器自动填充到 64 字节
};

// 方法4：Linux 内核宏
struct per_cpu_data {
    int a ____cacheline_aligned;
    int b ____cacheline_aligned;
};

// 方法5：每核独立结构体数组
struct alignas(64) per_cpu_stats {
    uint64_t order_count;
    uint64_t latency_sum;
    char pad[48];  // 显式填充到 64 字节
} stats[NUM_CPUS];
```

### 修复方法对比

| 方法 | 适用场景 | 优点 | 缺点 |
|------|----------|------|------|
| 手动 padding | 快速修复 | 不依赖编译器 | 需手算填充大小 |
| `aligned(64)` | GCC/Clang | 编译器自动处理 | 结构体增大 |
| C++11 `alignas` | C++11+ | 标准语法 | C 不可用 |
| `____cacheline_aligned` | Linux 内核 | 内核专用 | 依赖内核宏 |
| 每核独立结构体 | 多核统计 | 天然隔离 | 数组间距增大 |

### 伪共享 vs 真共享

| 类型 | 原因 | 是否有意的 | 性能影响 |
|------|------|-----------|----------|
| 真共享 | 不同核访问同一变量（需要同步） | 是（设计如此） | 可控（锁/原子操作） |
| 伪共享 | 不同核访问不同变量，但在同一 cache line | 否（应避免） | 不可控（隐藏延迟） |

### 检测方法

```bash
# 方法1：perf c2c（最精确）
perf c2c record ./program
perf c2c report
# 关注 HITM（Hit In Modified）指标

# 方法2：perf stat 观察 cache miss
perf stat -e cache-misses,cache-references ./program

# 方法3：perf record 采样
perf record -e LLC-load-misses ./program
perf report

# 方法4：Linux 内核 tracepoint
# /sys/kernel/debug/tracing/events/cache/
```

## HFT 关联

伪共享是 HFT 多核系统中最常见的性能陷阱。HFT 系统通常每核维护独立的统计计数器（如每核的订单计数），如果这些计数器在同一 cache line 中，每次更新都会触发 MESI 广播。

### HFT 典型场景

```c
// 错误：每核计数器在同一 cache line
struct {
    uint64_t order_count;     // offset 0
    uint64_t cancel_count;    // offset 8
    uint64_t latency_sum;     // offset 16
    uint64_t latency_count;   // offset 24
} per_cpu[4];  // 4 个核，每核一组计数器
// 问题：per_cpu[0] 和 per_cpu[1] 可能在同一 cache line

// 正确：每核独占 cache line
struct alignas(64) per_cpu_stats {
    uint64_t order_count;
    uint64_t cancel_count;
    uint64_t latency_sum;
    uint64_t latency_count;
    char pad[32];  // 填充到 64 字节
} per_cpu[4];
```

修复方法：用 `__attribute__((aligned(64)))` 让每核变量独占一个 cache line。Linux 内核的 `percpu` 变量就是通过类似机制避免伪共享。用 `perf c2c` 工具可以检测伪共享。

### 伪共享性能数据

| 场景 | 每次写延迟 | 1M 次/秒累计 | 影响 |
|------|-----------|-------------|------|
| 无伪共享（独立 cache line） | ~1ns | ~1ms | 无 |
| 伪共享（2 核争抢） | ~50-100ns | ~50-100ms | 50-100x 慢 |
| 伪共享（4 核争抢） | ~150-300ns | ~150-300ms | 150-300x 慢 |

## 自测题

1. **什么是伪共享？它为什么会导致性能下降？**

<details>
<summary>答案</summary>

伪共享：不同 CPU 核频繁写**同一 cache line 的不同变量**。因为 MESI 协议以 cache line 为单位管理一致性，写操作会让其他核的该行 invalidate。结果：cache line 在核间反复搬运（每次写都要重新加载），性能暴跌。虽然不同核写的是不同变量，但共享了 cache line → "伪"共享。
</details>

2. **如何修复伪共享？写出两种方法。**

<details>
<summary>答案</summary>

方法1：**手动 padding** 填充到 cache line 大小（64 字节）
```c
struct { int a; char pad[60]; int b; } data;
```

方法2：**GCC 属性对齐**
```c
struct data {
    int a __attribute__((aligned(64)));
    int b __attribute__((aligned(64)));
};
```

两种方法都让 a 和 b 在不同 cache line 中，消除伪共享。
</details>

3. **如何用 perf 工具检测伪共享？**

<details>
<summary>答案</summary>

用 `perf c2c`（Cache-to-Cache）工具检测：
```bash
perf c2c record ./program
perf c2c report
```
报告中的 "Hitm"（Hit in Modified）指标高表示跨核 cache line 竞争严重，可能存在伪共享。也可以用 `perf stat -e cache-misses` 观察 cache miss 异常高的区域。
</details>

4. **以下代码有什么伪共享问题？如何修复？**

```c
struct {
    int rx_packets;  // CPU0 收包线程写
    int tx_packets;  // CPU1 发包线程写
} nic_stats;
```

<details>
<summary>答案</summary>

问题：`rx_packets`（offset 0）和 `tx_packets`（offset 4）在同一 cache line（64 字节），CPU0 和 CPU1 频繁写不同变量但共享 cache line → 伪共享。

修复：
```c
struct {
    int rx_packets __attribute__((aligned(64)));
    int tx_packets __attribute__((aligned(64)));
} nic_stats;
// 或
struct { int rx_packets; char pad[60]; int tx_packets; } nic_stats;
```
</details>

## 参考与延伸

- [§16.1 MESI 协议](01-mesi.md) — 伪共享的底层原因
- [§16.5 实验要点](05-lab.md) — 实验 16-1 伪共享性能对比
- [Ch15 §15.4 关键概念](../../chapter-15-cache-basics/notes/section-0-本章完整概述.md) — Cache line 大小
