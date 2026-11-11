# 2.5 硬件并发数

> 第 2 章 · 上一节：[2.4 RAII 守卫](04-raii-guard.md) · 下一章：[第 3 章 共享数据](../ch03-sharing-data/README.md)

## 这节讲什么

`std::thread::hardware_concurrency()` 返回硬件线程数——决定线程池大小的参考值。但它只是**一句 hint**：可能返回 0、看不见容器配额与 CPU 亲和性、在异构核（P 核 + E 核）上把两种核一起数。实际最优线程数受多种因素影响。

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

> 标准原文（`[thread.thread.static]`）只有一句实质约束：
> **"This value should be considered only a hint."** —— 它是提示，不是承诺；
> 数值"无法计算 / 没有良定义"时，实现**应当返回 0**。
> 下面所有的坑都从这一句长出来：0 要兜底，而容器配额、亲和性、异构核它一概不管。

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

**修复**：`sysconf(_SC_NPROCESSORS_ONLN)` **解决不了这个问题**——它返回"在线 CPU 数"，
同样是宿主机的数（glibc 读 `/sys/devices/system/cpu/online`，不看 cgroup 配额）。
"我实际能用几个 CPU"得自己算，两件事取小：

| 限制来源 | 怎么查 | 典型场景 |
|---|---|---|
| CPU 亲和性 / cpuset | Linux `sched_getaffinity(0, …)` → `CPU_COUNT(...)` | `taskset`、k8s `cpuset.cpus` |
| cgroup v2 配额 | `/sys/fs/cgroup/cpu.max` → `<quota> <period>`，`max` 表示不限 | `--cpus=2` 落成 `200000 100000`（= 2 核） |
| cgroup v1 配额 | `cpu.cfs_quota_us` ÷ `cpu.cfs_period_us` | 老发行版 / k8s 旧集群 |

顺带一提：Go 1.25 才给 `GOMAXPROCS` 加了 cgroup 感知、JVM 有 `UseContainerSupport`——
都说明"标准接口看不见容器"是这一层的共性坑，不是 C++ 独有。

## 异构核：8 个逻辑核 ≠ 8 份算力（2026-11-11 实测，Apple M1 / macOS arm64 / clang 23.1）

上面讲的是 x86 的 SMT：**1 个物理核出 2 个同质逻辑核**，共享执行单元。ARM 这边是另一回事——
big.LITTLE / Apple Silicon 是**异构**：快核 + 慢核，指令集相同、宽度和频率差一截。
`hardware_concurrency()` 把两类核**一起数给你**，不给区分。

本机 `sysctl` 实测（M1，无 SMT）：

| 指标 | 命令 | 结果 |
|---|---|---|
| 芯片 | `machdep.cpu.brand_string` | Apple M1 |
| 逻辑核 | `hw.ncpu` | 8 |
| 物理核 | `hw.physicalcpu` | 8 |
| **P 核** | `hw.perflevel0.physicalcpu` | **4** |
| **E 核** | `hw.perflevel1.physicalcpu` | **4** |
| `hardware_concurrency()` | — | 8 |

注意这张表的错位：`physicalcpu` 也是 8（Apple 把每个核算 1 个物理核），
所以"按物理核数配线程"在这里照样得到 8——**但其中 4 个是 E 核**。

### 实测：4 线程 vs 8 线程（固定 300ms，4 条独立 xorshift 链，`-O2`）

```
4 线程 × 300ms，各线程完成块数：8180 8183 8234 8311     ← 4 个线程，调度器倾向放 P 核
8 线程 × 300ms，各线程完成块数：7600 7601 7612 7637 7676 7771 7786 7930
```

| 口径 | 4 线程 | 8 线程 | 变化 |
|---|---|---|---|
| 单线程吞吐（块/300ms） | ≈ 9000 | ≈ 7400 | **−18%** |
| 总算力 | 3.6 万 | 5.9 万 | **+64%** |

两个结论，方向相反但都对：

1. **吞吐型**任务该用满 8 个——E 核确实在贡献算力（反推 E 核单核约 P 核的 65%）。
2. **延迟敏感**任务该按 4 配线程——多开的那 4 个线程会被调度到 E 核，单个任务的完成时间被拉长。

### 一个容易误判的方法论坑

我第一版实验用"每线程做等量工作、比各自耗时"，结果 8 个线程耗时几乎一样（1.04x～1.10x 的近平分布），
**看不出异构**。原因是 macOS 调度器会把线程在 P/E 之间来回迁移：一条线程在 300ms 里既跑过 P 也跑过 E，
平均值被抹平了——**差的是平均值，不是极值**。所以判断异构核不能看"最慢线程有多慢"，
必须做**同负载的 A/B 对比**（4 线程 vs 8 线程的单线程吞吐）。

### 拿 P 核数个数的代码（本机实测 8 / 4）

```cpp
#include <sys/sysctl.h>
#include <thread>

static unsigned p_core_count() {
    int v = 0; size_t len = sizeof(v);
    if (sysctlbyname("hw.perflevel0.physicalcpu", &v, &len, nullptr, 0) == 0 && v > 0)
        return static_cast<unsigned>(v);
    return std::thread::hardware_concurrency();   // 老系统 / 同质核：退回提示值
}
```

实测输出：`hardware_concurrency() = 8`、`hw.perflevel0.physicalcpu(P核) = 4`。

> **Linux 侧（未实测，仅给口径）**：Pi 5 是 4×Cortex-A76 **同质**核，`nproc` 与
> `hardware_concurrency()` 预期都是 4，不存在本节问题；ARM 服务器（Graviton 等）目前也以同质为主。
> 真遇到异构 Linux，`/sys/devices/system/cpu/cpu*/cpu_capacity` 是内核暴露的算力权重，
> 调度器（EAS）就是按它决策的——这一条没在本机验证过。

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 接口 | `sysconf(_SC_NPROCESSORS_ONLN)` | `std::thread::hardware_concurrency()` |
| 可移植性 | 平台相关 | 跨平台标准 |
| 返回 0 | 不适用（返回 -1） | 标准（无法检测） |
| 语义 | 在线处理器数（含 E 核，不看 cgroup/affinity） | 硬件线程上下文数，**仅为提示** |

> 别把左边那列当"容器里的正确答案"：两者在容器里读到的是**同一个错数**（宿主机核数）。
> 要真实可用的 CPU，Linux 上得查 `sched_getaffinity` + cgroup（见错误 3 那张表）。

## HFT 关联

- **HFT 不依赖 `hardware_concurrency`**：HFT 固定线程数 + 绑核，手动指定哪个线程跑在哪个核。运行时检测不可靠（容器/NUMA/热迁移）。
- **NUMA 感知**：多路服务器有 NUMA——跨 NUMA 节点访问内存延迟翻倍。HFT 线程绑核 + 内存绑定到同 NUMA 节点。
- **超线程对 HFT 不友好**：同一物理核的两个超线程共享 L1/L2 cache 和执行单元，互相干扰。HFT 通常禁用超线程或只用一半线程绑物理核。
- **异构核按 P 核配线程**：Apple Silicon / ARM big.LITTLE 上 `hardware_concurrency()` 把 E 核一起数进去，
  实测 M1 开 8 线程单线程吞吐降 18%、总算力却升 64%——延迟敏感路径按 `hw.perflevel0.physicalcpu`（=4）配；
  macOS 没有绑核 API，唯一杠杆是**少开线程**（不开，就没有线程会被迁到 E 核）。

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

修复：容器里要查"实际能用的核"，不是查宿主机。cgroup v2 读配额：

```bash
cat /sys/fs/cgroup/cpu.max        # 例：200000 100000 → 2 核；"max" 表示不限
cat /sys/fs/cgroup/cpuset.cpus.effective   # 实际允许的 CPU 列表
```

v1 老环境是 `cpu.cfs_quota_us ÷ cpu.cfs_period_us`；代码里也可以用
`sched_getaffinity(0, …)` + `CPU_COUNT` 拿亲和性集合——两者取小才是真实可用并发度。

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

### Q5: M1 上 `hardware_concurrency()` 返回 8，按它开线程池有什么问题？

<details>
<summary>答案与复习指引</summary>

8 = **4 个 P 核 + 4 个 E 核**（`hw.perflevel0.physicalcpu` / `hw.perflevel1.physicalcpu` 各为 4），
不是 8 个等价核。实测（固定 300ms 吞吐）：4 线程时每线程 ≈9000 块，8 线程时 ≈7400 块——
**单线程吞吐 −18%**，总算力 +64%。

所以：吞吐型可以开 8；**延迟敏感必须按 4 配**，否则多出来的线程会被调度到 E 核上跑。
判据不是"看最慢线程"，而是同负载 A/B 对比 4 vs 8 的单线程吞吐（迁移会把极值抹平）。

复习：错误 3 之后的「异构核」整节，含 `sysctlbyname("hw.perflevel0.physicalcpu")` 取 P 核数。
</details>

---

## 参考与延伸

- 下一章：[第 3 章 共享数据](../ch03-sharing-data/README.md)
- 回到：[第 2 章](README.md)
