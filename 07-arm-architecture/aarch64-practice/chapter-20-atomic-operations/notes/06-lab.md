# §20.6 实验要点

> **来源：** [Ch20 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章以案例分析为主。关键案例：独占监视器工作原理、CAS 实现、WFE 自旋锁。本节给出每个案例的实验代码、性能测量方法和验证步骤。

## 核心要点

### 关键案例

| 案例 | 内容 | 关键知识点 |
|------|------|-----------|
| 独占监视器 | LDXR/STXR 的监视和清除机制 | 缓存行粒度监视 |
| CAS 实现 | LDXR+CMP+STXR 循环 | 原子比较交换 |
| WFE 自旋锁 | 低功耗自旋锁 | WFE/SEV 配合 |
| LSE 性能 | LDADD vs LDXR/STXR | 单指令 vs 循环 |

### 实验1：独占监视器竞争测试

```c
#include <stdatomic.h>
#include <time.h>
#include <stdio.h>

// 高竞争 CAS 性能测试
void bench_cas_contention(atomic_int *counter, int iterations) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        int expected;
        do {
            expected = atomic_load(counter);
        } while (!atomic_compare_exchange_weak(counter, &expected, expected + 1));
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double ns = (end.tv_sec - start.tv_sec) * 1e9 +
                (end.tv_nsec - start.tv_nsec);
    printf("CAS: %.2f ns/op (contention)\n", ns / iterations);
}

// 低竞争（每核独立地址）
void bench_cas_no_contention(atomic_int *counter, int iterations) {
    // 单线程，无竞争
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        int expected = atomic_load(counter);
        atomic_compare_exchange_weak(counter, &expected, expected + 1);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double ns = (end.tv_sec - start.tv_sec) * 1e9 +
                (end.tv_nsec - start.tv_nsec);
    printf("CAS: %.2f ns/op (no contention)\n", ns / iterations);
}
```

### 实验2：WFE vs 普通自旋锁对比

```c
#include <pthread.h>
#include <stdatomic.h>

atomic_int lock = 0;
atomic_int counter = 0;

// 普通自旋锁
void spin_lock_normal(void) {
    while (atomic_exchange(&lock, 1)) ;
}
void spin_unlock_normal(void) {
    atomic_store(&lock, 0);
}

// WFE 自旋锁（ARM 汇编）
void spin_lock_wfe(void) {
    asm volatile(
        "1: sevl\n"
        "2: wfe\n"
        "   ldaxr w1, [%0]\n"
        "   cbnz w1, 2b\n"
        "   stxr w2, wzr, [%0]\n"  // 尝试获取（写 0... 其实写 1）
        "   cbnz w2, 1b\n"
        :: "r"(&lock) : "w1", "w2"
    );
}
void spin_unlock_wfe(void) {
    asm volatile("stlr wzr, %0\n sev" :: "Q"(lock));
}

// 对比两种锁
void bench_lock(void (*lock_fn)(void), void (*unlock_fn)(void),
                int iterations) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        lock_fn();
        counter++;
        unlock_fn();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    // ... 打印延迟
}
```

### 推荐实践

1. 在 QEMU 双核上测试 LDXR/STXR 的竞争行为
2. 对比普通自旋锁和 WFE 自旋锁的功耗/性能
3. 测试 LSE（如果 QEMU 支持 ARMv8.1）vs LDXR/STXR 的性能差异
4. 用 `perf stat` 测量原子操作的周期数

### 预期性能数据

| 操作 | 低竞争 | 高竞争(2核) | 高竞争(4核) |
|------|--------|------------|------------|
| LDXR/STXR CAS | ~10-15ns | ~30-50ns | ~50-100ns |
| LSE CAS | ~5ns | ~5-10ns | ~5-10ns |
| 普通自旋锁 | ~50ns | ~200ns | ~500ns |
| WFE 自旋锁 | ~60ns | ~100ns | ~200ns |
| SPSC push/pop | ~5-10ns | — | — |

## HFT 关联

这些案例直接对应 HFT 无锁编程的实际应用。独占监视器的缓存行粒度知识帮助避免伪共享导致的原子操作性能下降。CAS 实现是 HFT 订单簿并发更新的基础。WFE 自旋锁在 HFT 中可以减少等待时的功耗和总线干扰。

### HFT 性能验证建议

```bash
# 1. 在 Pi5 上编译（启用 LSE）
gcc -O2 -mcpu=cortex-a76 -o atomic_bench atomic_bench.c -lpthread

# 2. 限制在 2 核上运行
taskset -c 0,1 ./atomic_bench

# 3. 用 perf 测量
perf stat -e cycles,inst_retired,cache-misses ./atomic_bench

# 4. 验证 LSE 是否启用
objdump -d atomic_bench | grep -E "ldadd|cas[0-9]"

# 5. 检查 CPU 特性
cat /proc/cpuinfo | grep Features
```

建议在 Pi5（支持 LSE）上对比 LSE 和 LDXR/STXR 的性能差异——在高竞争场景下 LSE 的优势最明显。

## 自测题

1. **如何在 QEMU 上测试独占监视器的竞争行为？**

<details>
<summary>答案</summary>

启动两个核，都对同一地址执行 LDXR/STXR 循环（如原子计数器自增）。用 `CNTPCT_EL0` 测量每次成功的 STXR 需要多少 cycle。在高竞争下，STXR 失败率升高，每次成功需要更多重试，cycle 数增加。对比低竞争（每核独立地址）和高竞争（同一地址）的性能差异。
</details>

2. **如何对比普通自旋锁和 WFE 自旋锁的性能？**

<details>
<summary>答案</summary>

编写两个版本的自旋锁（普通 LDXR/STXR 循环 vs WFE 版本），在多核上并发获取/释放锁。测量：
1. 获取锁的平均延迟（cycle 数）
2. 总线带宽消耗（普通自旋锁的 LDXR 消耗更多带宽）
3. 功耗（WFE 版本更低，需硬件功耗计）

预期结果：WFE 版本功耗更低、总线带宽更少，但获取延迟可能略高（WFE 唤醒延迟）。
</details>

3. **如何验证 LSE 是否被启用？**

<details>
<summary>答案</summary>

1. 编译时用 `-march=armv8.1-a` 或 `-mcpu=cortex-a76`
2. 反汇编目标代码，查找 `LDADD`/`CAS`/`SWP` 等 LSE 指令（而非 LDXR/STXR 循环）
3. 或运行时读 `ID_AA64ISAR0_EL1` 寄存器的 atomic 字段（bit[23:20]），非零表示支持 LSE
4. Linux 中查 `/proc/cpuinfo` 的 Features 是否包含 `atomics`
</details>

4. **CAS 在高竞争时性能下降的原因是什么？如何用 LSE 改善？**

<details>
<summary>答案</summary>

CAS 高竞争下降原因：多个核同时 LDXR 同一地址，只有一个 STXR 成功，其他失败重试 → 重试越多延迟越大（livelock）。

LSE 改善：LSE 的 CAS 指令（如 `CAS`）是单指令原子操作，硬件保证原子完成，无循环重试。延迟固定 ~5-10ns，不受竞争影响。编译时用 `-march=armv8.1-a` 让编译器使用 LSE CAS 替代 LDXR/STXR 循环。
</details>

## 参考与延伸

- [§20.1 独占监视器](01-exclusive-monitor.md) — 实验基础
- [§20.3 ARMv8.1 LSE](03-lse.md) — LSE 性能对比
- [§20.4 WFE/SEV](04-wfe-sev.md) — WFE 自旋锁实现
