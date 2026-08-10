# §18.6 实验要点

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章以案例分析为主，无独立编号实验。关键案例：消息传递、自旋锁、邮箱传递、DMA、IC 失效。本节给出每个案例的实验代码和验证方法。

## 核心要点

### 关键案例

| 案例 | 屏障使用 | 核心知识点 |
|------|----------|-----------|
| 消息传递 | `dmb ishst` + `dmb ishld` | Store-Store + Load-Load 屏障配对 |
| 自旋锁 | `dmb ish` | 获取后+释放前全屏障 |
| DMA | `dsb sy` | DSB 完全停住 |
| IC 失效 | `dsb ish` + `isb` | TLB 刷新后 DSB+ISB |

### 消息传递实验代码

```c
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

volatile int data = 0;
volatile int flag = 0;

// 版本1：不加屏障（ARM 上可能失败）
void *producer_no_barrier(void *arg) {
    data = 42;
    flag = 1;  // ARM 可能重排：flag=1 先于 data=42
    return NULL;
}

void *consumer_no_barrier(void *arg) {
    while (flag != 1) ;
    if (data != 42) printf("BUG: flag=1 but data=%d\n", data);
    return NULL;
}

// 版本2：加 DMB 屏障
void *producer_dmb(void *arg) {
    data = 42;
    asm volatile("dmb ishst" ::: "memory");
    flag = 1;
    return NULL;
}

void *consumer_dmb(void *arg) {
    while (flag != 1) ;
    asm volatile("dmb ishld" ::: "memory");
    if (data != 42) printf("BUG: flag=1 but data=%d\n", data);
    return NULL;
}

// 版本3：用 STLR/LDAR（最优）
void *producer_stlr(void *arg) {
    data = 42;
    asm volatile("stlr %w0, %1" :: "r"(1), "Q"(flag));
    return NULL;
}

void *consumer_ldar(void *arg) {
    int f;
    do {
        asm volatile("ldar %w0, %1" : "=r"(f) : "Q"(flag));
    } while (f != 1);
    if (data != 42) printf("BUG\n");
    return NULL;
}
```

### 推荐实践

1. 在 QEMU 多核上跑消息传递实验，不加屏障 vs 加屏障对比结果
2. 用 `perf stat` 对比有屏障/无屏障的性能差异
3. 阅读 Linux `arch/arm64/include/asm/barrier.h` 理解 API 实现

### 性能对比实验

```c
#include <time.h>
#include <stdatomic.h>

#define ITERATIONS 1000000

void bench_dmb(void) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ITERATIONS; i++) {
        asm volatile("dmb ishst" ::: "memory");
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("DMB ishst: %.2f ns/op\n",
        (end.tv_sec - start.tv_sec) * 1e9 / ITERATIONS +
        (end.tv_nsec - start.tv_nsec) / (double)ITERATIONS);
}

void bench_stlr(void) {
    struct timespec start, end;
    volatile int target = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ITERATIONS; i++) {
        asm volatile("stlr %w0, %1" :: "r"(i), "Q"(target));
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("STLR: %.2f ns/op\n",
        (end.tv_sec - start.tv_sec) * 1e9 / ITERATIONS +
        (end.tv_nsec - start.tv_nsec) / (double)ITERATIONS);
}
```

### 屏障性能预期数据

| 屏障 | 指令 | 预期延迟 (A76) | 说明 |
|------|------|---------------|------|
| 无 | — | ~1ns | 基准 |
| `dmb ishst` | 1 | ~3-5ns | Store-Store |
| `dmb ish` | 1 | ~5-10ns | 全屏障 |
| `dsb sy` | 1 | ~50-100ns | 完全停住 |
| `STLR` | 1 | ~5ns | Store-Release |
| `LDAR` | 1 | ~5ns | Load-Acquire |

## HFT 关联

消息传递案例是 HFT SPSC 无锁队列的原型——在 QEMU 上验证"不加屏障消费者读到旧数据"的现象，可以直观理解 ARM 弱序模型的影响。

> **注意**：QEMU 的内存模型可能比真实硬件更强（QEMU 不完全模拟弱序）。建议在 Pi5 上做实际测试。

### HFT 实验建议

```bash
# 1. 在 Pi5 上编译运行消息传递实验
gcc -O2 -lpthread -o mp_test mp_test.c
taskset -c 0,1 ./mp_test

# 2. 用 perf 测量屏障开销
perf stat -e cycles,inst_retired ./bench_dmb
perf stat -e cycles,inst_retired ./bench_stlr

# 3. 对比有/无屏障的消息传递
./mp_no_barrier  # ARM 上可能看到 BUG
./mp_with_barrier  # 应该正确
```

性能对比数据（有屏障 vs 无屏障的延迟差）可以帮助量化屏障开销，在 HFT 系统中做出正确的性能-正确性权衡。

## 自测题

1. **如何在 QEMU 上验证 ARM 弱序内存模型的影响？**

<details>
<summary>答案</summary>

编写消息传递测试：两个核，核 A 写 data 后写 flag，核 B 读 flag 后读 data。
- 不加屏障：多次运行，观察核 B 偶尔读到 data 旧值（flag=1 但 data≠42）
- 加屏障（`dmb ishst` + `dmb ishld`）：核 B 总是读到 data=42

注意：QEMU 可能不完全模拟弱序，建议在真实 Pi5 上测试更可靠。
</details>

2. **`perf stat` 能否测量屏障指令的开销？**

<details>
<summary>答案</summary>

可以间接测量。用 `perf stat` 对比有屏障和无屏障版本的执行时间差异：
```bash
perf stat ./no_barrier_version
perf stat ./with_barrier_version
```
差异主要来自 DMB/DSB 的停顿周期。也可以用 `perf stat -e cycles,inst_retired` 对比 IPC 变化。注意缓存效应可能干扰测量，需要多次运行取平均。
</details>

3. **阅读 Linux `barrier.h` 应该关注什么？**

<details>
<summary>答案</summary>

关注：
1. `smp_mb`/`smp_rmb`/`smp_wmb` 的定义（展开为什么 ARM 指令）
2. `__smp_mb()` vs `mb()` 的区别（SMP vs DMA）
3. `barrier()` 的定义（编译器屏障）
4. `__smp_store_release()` / `__smp_load_acquire()` 是否用 LDAR/STLR
5. 不同 ARM 版本（ARMv8.0 vs 8.1+）的屏障实现差异
</details>

4. **在 Pi5 上对比 DMB ishst 和 STLR 的延迟，预期结果是什么？**

<details>
<summary>答案</summary>

预期 STLR 比 DMB ishst + STR 快约 2-5ns：
- `dmb ishst` + `STR`：~8ns（两条指令 + DMB 停顿）
- `STLR`：~5ns（单指令 + CPU 更精确优化）

原因：STLR 是单点屏障，CPU 知道只需要保证这一个 Store 的 release 语义，比全屏障 DMB 停顿更少。
</details>

## 参考与延伸

- [§18.3 典型场景](03-typical-scenarios.md) — 各场景的屏障使用
- [§18.7 易错点](07-pitfalls.md) — 屏障使用常见错误
- [Ch19 全章](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — Linux 内核中的真实屏障案例
