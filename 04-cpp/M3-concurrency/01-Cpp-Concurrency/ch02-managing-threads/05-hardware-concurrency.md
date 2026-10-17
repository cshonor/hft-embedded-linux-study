# 2.5 硬件并发数

> 第 2 章 · 上一节：[2.4 RAII 守卫](04-raii-guard.md) · 下一章：[第 3 章 共享数据](../ch03-sharing-data/README.md)

## 这节讲什么

`std::thread::hardware_concurrency()` 返回硬件线程数——决定线程池大小的参考值。但它只是参考，实际最优线程数受多种因素影响。

## 为什么要学这个（先建立直觉）

C 程序员可能用 `sysconf` 或 `/proc/cpuinfo` 获取 CPU 核心数：

```c
// C：获取 CPU 核心数
long n = sysconf(_SC_NPROCESSORS_ONLN);  // Linux
// 或 GetSystemInfo(&si); si.dwNumberOfProcessors;  // Windows
printf("CPU cores: %ld\n", n);
```

C++11 标准化了这个接口：

```cpp
// C++：标准接口
unsigned n = std::thread::hardware_concurrency();
// 返回硬件线程数（含超线程），可能返回 0
```

但这个数值只是"硬件能力"的参考，不等于"最优线程数"：

```cpp
// 错误：直接用 hardware_concurrency 作为线程数
unsigned n = std::thread::hardware_concurrency();
for (unsigned i = 0; i < n; i++)
    pool.emplace_back(worker);
// 问题：没有考虑 IO 等待、缓存竞争、NUMA 等
```

## 核心用法详解

### 基本用法

```cpp
#include <thread>

unsigned int n = std::thread::hardware_concurrency();
// 返回值含义：
// - 通常 = 物理核心 × 超线程因子（如 8 核 16 线程 CPU 返回 16）
// - 可能返回 0（标准允许，表示无法检测）
// - 不保证准确（虚拟机/容器可能返回错误值）

if (n == 0) n = 4;  // fallback
std::cout << "Recommended threads: " << n << '\n';
```

### 线程数选择策略

```cpp
unsigned choose_thread_count(bool cpu_intensive, bool io_intensive) {
    unsigned hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 4;

    if (cpu_intensive) {
        // CPU 密集型：线程数 = 核心数（多了反而降低性能）
        return hw;
    } else if (io_intensive) {
        // IO 密集型：线程数 > 核心数（IO 等待时切换到其他线程）
        return hw * 2;  // 或更多，取决于 IO/计算比
    } else {
        // 混合型：从核心数开始，压测调整
        return hw;
    }
}
```

### CPU 密集型 vs IO 密集型

| 任务类型 | 最优线程数 | 原因 |
|----------|-----------|------|
| CPU 密集 | ≈ 核心数 | 多了竞争 CPU，上下文切换浪费 |
| IO 密集 | > 核心数 | IO 等待时 CPU 空闲，可切换到其他线程 |
| 混合型 | 压测决定 | 无公式，需实际测试 |

## 常见错误（新手踩坑）

### 错误 1：CPU 密集型任务开太多线程

```cpp
// 错误：8 核 CPU 开 100 个 CPU 密集线程
unsigned hw = std::thread::hardware_concurrency();  // 16
unsigned n = 100;  // 远超核心数
for (unsigned i = 0; i < n; i++)
    pool.emplace_back(cpu_heavy_task);
// 结果：上下文切换开销 > 并行收益，比 16 线程还慢
```

**修复**：CPU 密集型线程数 ≈ `hardware_concurrency()`。

### 错误 2：不处理返回 0

```cpp
// 错误：不处理 hardware_concurrency() 返回 0
unsigned n = std::thread::hardware_concurrency();
std::vector<std::thread> pool;
for (unsigned i = 0; i < n; i++)  // n=0 → 不创建任何线程
    pool.emplace_back(worker);
// 串行执行，性能不如预期，但无错误提示
```

**修复**：`if (n == 0) n = 4;` 或其他合理 fallback。

### 错误 3：容器/Docker 环境下的误导

```cpp
// 容器中 hardware_concurrency() 可能返回宿主机核心数
// 而非容器限制的 CPU 配额
unsigned n = std::thread::hardware_concurrency();
// Docker 限制 2 核，但返回 64（宿主机核心数）
// 开 64 个线程 → 严重过度订阅
```

**修复**：容器环境用 `sysconf(_SC_NPROCESSORS_ONLN)` 或读 cgroup 限制。

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 接口 | `sysconf(_SC_NPROCESSORS_ONLN)` | `std::thread::hardware_concurrency()` |
| 可移植性 | 平台相关 | 跨平台标准 |
| 返回 0 | 不适用（返回 -1） | 标准（无法检测） |
| 语义 | 在线处理器数 | 硬件并发支持的线程数 |

## HFT 关联

- **HFT 不依赖 `hardware_concurrency`**：HFT 固定线程数 + 绑核，手动指定哪个线程跑在哪个核。运行时检测不可靠（容器/NUMA/热迁移）。
- **NUMA 感知**：多路服务器有 NUMA——跨 NUMA 节点访问内存延迟翻倍。HFT 线程绑核 + 内存绑定到同 NUMA 节点。
- **超线程对 HFT 不友好**：同一物理核的两个超线程共享 L1/L2 cache 和执行单元，互相干扰。HFT 通常禁用超线程或只用一半线程绑物理核。

## 代码自测

### Q1: 下列代码在 8 核 16 线程 CPU 上返回什么？

```cpp
std::cout << std::thread::hardware_concurrency();
```

<details>
<summary>答案与复习指引</summary>

**通常返回 16**（8 物理 × 2 超线程）。但也可能返回 8（某些实现只算物理核）或 0（无法检测）。

标准只说"硬件并发支持的线程数"，具体含义由实现决定。不能假设它等于物理核心数。

复习：`hardware_concurrency()` 返回硬件线程数（含超线程），不是物理核心数。
</details>

### Q2: 下列代码有什么问题？

```cpp
// 8 核 CPU，CPU 密集型任务
unsigned n = std::thread::hardware_concurrency() * 4;  // 32 线程
for (unsigned i = 0; i < n; i++)
    pool.emplace_back(matrix_multiply);
```

<details>
<summary>答案与复习指引</summary>

**过度订阅**。CPU 密集型任务开 32 个线程在 8 核 CPU 上，大量时间浪费在上下文切换和 cache 抖动上，比 8 线程更慢。

修复：CPU 密集型线程数 ≈ `hardware_concurrency()`（或物理核心数）。

复习：CPU 密集型线程数 = 核心数。多了反而降低性能。
</details>

### Q3: Docker 容器中 `hardware_concurrency()` 返回 64，但容器限制 2 核，会发生什么？

<details>
<summary>答案与复习指引</summary>

**过度订阅**。函数返回宿主机的 64 核，但容器只分配 2 核。创建 64 个线程会严重竞争 CPU，每个线程只能分到 1/32 核的算力。

修复：容器中读 cgroup CPU 配额：
```bash
cat /sys/fs/cgroup/cpu/cpu.cfs_quota_us  # 配额
cat /sys/fs/cgroup/cpu/cpu.cfs_period_us  # 周期
# 线程数 = quota / period
```

复习：`hardware_concurrency()` 在容器中不可靠——返回宿主机核心数而非容器配额。
</details>

### Q4: 为什么 HFT 禁用超线程？

<details>
<summary>答案与复习指引</summary>

超线程的两个逻辑核共享：
1. **L1/L2 cache**：一个线程的 cache miss 驱逐另一个线程的数据
2. **执行单元**：浮点单元、ALU 可能竞争
3. **LLC（L3）**：跨超线程的 cache line 互相干扰

HFT 需要确定性——超线程导致的 cache 干扰引入不可预测的延迟波动。禁用超线程 + 每物理核绑一个 HFT 线程，获得最稳定的延迟。

复习：超线程提升吞吐量但牺牲单线程延迟确定性。HFT 选确定性。
</details>

---

## 参考与延伸

- 下一章：[第 3 章 共享数据](../ch03-sharing-data/README.md)
- 回到：[第 2 章](README.md)
