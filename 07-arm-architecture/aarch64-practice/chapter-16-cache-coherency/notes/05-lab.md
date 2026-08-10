# §16.5 实验要点

> **来源：** [Ch16 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章 2 个实验：高速缓存伪共享性能对比、使用 Perf C2C 发现伪共享。通过性能数据直观感受伪共享的影响，掌握检测和定位伪共享的工具链。

## 核心要点

| 实验 | 内容 | 平台 | 关键技能 |
|------|------|------|----------|
| 16-1 | 高速缓存伪共享（性能对比） | Linux | 伪共享 vs 对齐的性能差异 |
| 16-2 | 使用 Perf C2C 发现伪共享 | Linux | perf c2c 工具使用 |

### 实验 16-1 性能对比

```c
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

#define ITERATIONS 100000000

// 有伪共享
struct {
    int a;
    int b;
} data_false_sharing;

// 无伪共享（手动 padding）
struct {
    int a;
    char pad[60];
    int b;
} data_aligned;

// 无伪共享（属性对齐）
struct {
    int a __attribute__((aligned(64)));
    int b __attribute__((aligned(64)));
} data_attr;

void *writer_a(void *arg) {
    volatile int *p = (volatile int *)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        *p = i;
    }
    return NULL;
}

void *writer_b(void *arg) {
    volatile int *p = (volatile int *)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        *p = i;
    }
    return NULL;
}

double run_test(int *pa, int *pb) {
    pthread_t t1, t2;
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_create(&t1, NULL, writer_a, pa);
    pthread_create(&t2, NULL, writer_b, pb);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

int main() {
    printf("False sharing: %.2f ms\n", 
        run_test(&data_false_sharing.a, &data_false_sharing.b));
    printf("Manual pad:    %.2f ms\n", 
        run_test(&data_aligned.a, &data_aligned.b));
    printf("Attr aligned:  %.2f ms\n", 
        run_test(&data_attr.a, &data_attr.b));
    return 0;
}
```

### 实验 16-1 预期结果

| 模式 | 预期延迟（2核） | 预期延迟（4核） | 加速比 |
|------|----------------|----------------|--------|
| False sharing | ~5000-8000ms | ~8000-12000ms | 1x（基准） |
| Manual pad | ~300-500ms | ~300-500ms | 10-25x |
| Attr aligned | ~300-500ms | ~300-500ms | 10-25x |

### 实验 16-2 Perf C2C

```bash
# 1. 编译伪共享测试程序
gcc -O2 -o false_sharing test_false_sharing.c -lpthread

# 2. 采集 cache-to-cache 传输数据
sudo perf c2c record ./false_sharing

# 3. 查看报告
sudo perf c2c report
# 关注以下指标：
#   - HITM（Hit In Modified）：跨核 M 状态 cache line 读取
#   - HITM 百分比高 = 伪共享严重
#   - 报告会显示具体地址和 cache line 偏移

# 4. 也可以用 perf stat 观察 cache miss
perf stat -e cache-misses,cache-references,LLC-load-misses ./false_sharing

# 5. perf record 采样分析
perf record -e LLC-load-misses ./false_sharing
perf report
```

### Perf C2C 报告解读

| 指标 | 含义 | 伪共享信号 |
|------|------|-----------|
| HITM | Hit In Modified 状态 | 高 = 跨核争抢 cache line |
| HITM% | HITM / 总 HIT | > 20% = 严重伪共享 |
| Remote HITM | 跨 socket 的 M 状态读 | 多 socket 系统关注 |
| Store Retired | 退休的写操作 | 定位热变量 |
| Data Address | cache line 地址 | 定位伪共享的具体地址 |

### 编译运行命令

```bash
# 编译（需要 -lpthread）
gcc -O2 -o false_sharing test_false_sharing.c -lpthread

# 运行
./false_sharing

# 限制在 2 个核上运行
taskset -c 0,1 ./false_sharing

# 限制在 4 个核上运行
taskset -c 0,1,2,3 ./false_sharing
```

## HFT 关联

实验 16-1 的性能对比数据对 HFT 开发者很有说服力——伪共享可以让性能下降 5-10 倍。HFT 系统中的每核统计计数器是最容易产生伪共享的地方。

### HFT 每核统计计数器模板

```c
// 正确的 HFT 每核统计结构
struct alignas(64) hft_per_cpu_stats {
    uint64_t order_count;
    uint64_t cancel_count;
    uint64_t latency_sum_ns;
    uint64_t latency_count;
    uint64_t error_count;
    char pad[24];  // 填充到 64 字节
} __attribute__((aligned(64)));

static struct hft_per_cpu_stats per_cpu_stats[MAX_CPUS];

// 每核只更新自己的计数器
void on_order(int cpu) {
    per_cpu_stats[cpu].order_count++;
    // 不影响其他核的 cache line
}
```

实验 16-2 的 perf c2c 工具是定位伪共享的利器，在生产环境中如果发现延迟抖动，可以用 perf c2c 检查是否有伪共享。在 Pi5 多核上，这个实验特别有意义——A76 的 L1/L2 私有，cache line 跨核传输开销显著。

## 自测题

1. **实验 16-1 中，伪共享版本比对齐版本慢多少？为什么？**

<details>
<summary>答案</summary>

伪共享版本通常比对齐版本慢 **5-10 倍**（具体取决于核数和写入频率）。原因：每次写操作都导致 cache line 在核间传输（MESI invalidate → reload），每次传输 ~50-100ns。对齐后每核独立 cache line，无跨核传输。
</details>

2. **Perf C2C 报告中哪个指标反映伪共享？**

<details>
<summary>答案</summary>

**HITM**（Hit In Modified）指标。HITM 高表示一个核频繁从另一个核的 M 状态 cache line 读取数据（跨核传输），是伪共享的强烈信号。报告还会显示具体地址和 cache line 偏移，帮助定位到具体变量。
</details>

3. **如何在 HFT 代码中预防伪共享？**

<details>
<summary>答案</summary>

1. 每核变量用 `__attribute__((aligned(64)))` 对齐到 cache line
2. 使用 `struct { char pad[64]; int counter; } per_cpu[N_CPUS];` 模式
3. 热数据结构避免紧凑布局，用 padding 分隔不同核访问的字段
4. 用 `perf c2c` 定期检查
5. 使用 Linux `percpu` 变量机制（内核场景）
</details>

## 参考与延伸

- [§16.2 伪共享](02-false-sharing.md) — 伪共享原理和修复
- [§16.1 MESI 协议](01-mesi.md) — 伪共享的底层机制
- [Ch15 §15.3 Cache 层次](../../chapter-15-cache-basics/notes/section-0-本章完整概述.md) — 多核 cache 架构
