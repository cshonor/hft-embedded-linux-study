# §17.6 实验要点

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章以案例分析为主，无独立编号实验。关键案例：Linux 内核 TLB 维护、ASID 切换、BBM 机制。通过阅读内核源码和 QEMU 实验深入理解 TLB 管理。

## 核心要点

### 关键案例

| 案例 | 内容 | 关键点 |
|------|------|--------|
| Linux TLB 维护 | 内核中 TLB 刷新的时机和方式 | vae1is vs alle1is 选择 |
| ASID 切换 | 进程切换时 ASID 设置 | TTBR0 高位写 ASID |
| BBM 机制 | 修改页表的安全协议 | break → flush → DSB → make |
| 大页实验 | 大页 vs 小页 TLB miss | TLB miss 计数对比 |

### Linux 内核源码阅读

```bash
# 1. TLB 刷新核心代码
# arch/arm64/mm/tlb.S — TLB 刷新汇编实现
#   __tlb_switch_to_guest  — KVM guest 切换
#   __tlb_flush_all        — 全刷
#   __flush_tlb_range      — 按范围刷

# 2. 进程切换
# arch/arm64/mm/context.c — ASID 分配和切换
#   check_and_switch_context — 进程切换设置 TTBR0

# 3. 页表操作
# arch/arm64/include/asm/pgtable.h
#   set_pte_at — 封装 BBM 逻辑

# 4. TLB API 定义
# arch/arm64/include/asm/tlbflush.h
#   flush_tlb_all / flush_tlb_mm / flush_tlb_page / flush_tlb_range
```

### 实验代码：手动 TLB 操作

```c
// 在裸金属代码中手动修改页表 + TLB 刷新
// 修改一个 VA 的映射，验证 BBM

void change_mapping(pte_t *pte, unsigned long va, pte_t new_pte) {
    // BBM: break before make
    // Step 1: break（设 Invalid）
    *pte = 0;
    asm volatile("dc cvac, %0" :: "r"(pte));  // clean D-cache
    
    // Step 2: flush TLB（本核）
    asm volatile("tlbi vae1, %0" :: "r"(va));
    asm volatile("dsb sy" ::: "memory");
    
    // Step 3: make（写新映射）
    *pte = new_pte;
    asm volatile("dc cvac, %0" :: "r"(pte));  // clean D-cache
    
    // Step 4: flush TLB + ISB
    asm volatile("tlbi vae1, %0" :: "r"(va));
    asm volatile("dsb sy" ::: "memory");
    asm volatile("isb" ::: "memory");
}
```

### 实验代码：TLB miss 测量

```c
// 通过性能计数器测量 TLB miss
#include <perfmon/perf_event.h>

// 设置性能计数器
struct perf_event_attr attr;
attr.type = PERF_TYPE_HARDWARE;
attr.size = sizeof(attr);
attr.config = PERF_COUNT_HW_CACHE_MISSES;  // 或用 RAW 事件
attr.disabled = 0;
attr.exclude_kernel = 1;

int fd = perf_event_open(&attr, 0, -1, -1, 0);

// 测量大页 vs 小页
void measure_tlb_miss(void *buf, size_t size, int huge_page) {
    // 访问所有页
    char *p = (char *)buf;
    size_t step = huge_page ? 2*1024*1024 : 4096;
    
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    for (size_t off = 0; off < size; off += step) {
        asm volatile("ldr x0, [%0]" :: "r"(p + off));
    }
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    
    long long miss_count;
    read(fd, &miss_count, sizeof(miss_count));
    printf("TLB miss: %lld (%s page)\n", miss_count, 
           huge_page ? "huge" : "small");
}
```

### QEMU 实验命令

```bash
# 使用 QEMU 观察 TLB 操作
# -d int：打印中断/异常信息
# -d mmu：专门查看 MMU/TLB 操作
# -d page_dumps：打印页表操作

qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a72 \
    -d int -D qemu_log.txt \
    -kernel baremetal.elf \
    -nographic

# 查看 TLB 相关日志
grep -i "tlb\|page" qemu_log.txt

# 单步调试观察 TLB
qemu-system-aarch64 \
    -machine virt -cpu cortex-a72 \
    -s -S \                    # 等待 GDB 连接
    -kernel baremetal.elf \
    -nographic
# 另一个终端：
gdb -ex "target remote :1234" baremetal.elf
```

### Linux 大页实验

```bash
# 1. 检查大页支持
cat /proc/meminfo | grep -i huge
# HugePages_Total:     0
# Hugepagesize:     2048 kB

# 2. 预留大页
echo 100 > /proc/sys/vm/nr_hugepages

# 3. 使用 mmap 大页
# 见上方 C 代码

# 4. 透明大页
echo always > /sys/kernel/mm/transparent_hugepage/enabled

# 5. 查看大页使用
cat /proc/meminfo | grep -i huge
```

## HFT 关联

虽然本章无独立实验，但 TLB 管理知识在 HFT 中有实际应用：1) 用大页减少 TLB miss；2) 避免 `mprotect`/`munmap` 触发 TLB 刷新；3) 线程绑定 CPU 避免进程切换 TLB flush。

建议在 QEMU 上实验手动 TLB 操作（`tlbi vae1`），理解 TLB 刷新对后续访存的影响——刷新后第一次访问会 TLB miss，延迟增加。

### HFT TLB 优化验证实验

```c
// 对比 4KB 页 vs 2MB 大页的延迟
#define BENCH_SIZE (256 * 1024 * 1024)  // 256MB
#define PAGE_4K    4096
#define PAGE_2M    (2 * 1024 * 1024)

void bench_tlb_latency(void *buf, size_t step) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // 顺序访问每页（触发 TLB miss）
    char *p = (char *)buf;
    for (size_t off = 0; off < BENCH_SIZE; off += step) {
        asm volatile("ldr x0, [%0]" :: "r"(p + off));
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Step %zu: %.2f ms\n", step, 
           (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1e6);
}

// 预期结果：
//   Step 4KB:  较慢（大量 TLB miss）
//   Step 2MB:  较快（少 TLB miss）
```

## 自测题

1. **Linux 的 `arch/arm64/mm/tlb.S` 中 `flush_tlb_mm` 函数用什么 TLBI 指令？**

<details>
<summary>答案</summary>

用 `tlbi aside1is, x0`（Inner Shareable，按 ASID 刷）。因为 `flush_tlb_mm` 刷整个进程的 TLB（不是单个 VA），用 ASID 精确刷该进程。`is`（Inner Shareable）确保所有核上的该 ASID 条目都被刷新。
</details>

2. **如何用 QEMU 观察 TLB 操作？**

<details>
<summary>答案</summary>

用 `-d int` 选项打印中断/异常信息，包括 TLB 相关操作：
```bash
qemu-system-aarch64 -d int -D qemu_log.txt ...
```
日志中可以看到 TLB miss、页表 walk 等信息。也可以用 `-d mmu` 选项专门查看 MMU/TLB 操作。
</details>

3. **在裸金属代码中如何验证 BBM 的必要性？**

<details>
<summary>答案</summary>

实验：两个核，核 A 修改页表映射（VA→PA1 改为 VA→PA2），核 B 持续读该 VA。
- 不遵循 BBM（直接改）：核 B 可能在 TLB 刷新前读到 PA1 的旧数据
- 遵循 BBM（break→flush→DSB→make）：核 B 在 break 后 TLB miss，等到 make 后才读到 PA2 的新数据

对比两种情况的核 B 读到的值，验证 BBM 的必要性。
</details>

4. **如何在 Linux 上验证大页减少 TLB miss？**

<details>
<summary>答案</summary>

1. 预留大页：`echo 100 > /proc/sys/vm/nr_hugepages`
2. 用 `mmap(MAP_HUGETLB)` 分配大页内存和普通内存
3. 用 `perf stat -e dTLB-load-misses` 分别测量两种情况的 TLB miss
4. 大页版本 TLB miss 应远少于小页版本

```bash
perf stat -e dTLB-load-misses ./test_hugepage
perf stat -e dTLB-load-misses ./test_smallpage
```
</details>

## 参考与延伸

- [§17.3 TLB 刷新指令](03-tlb-flush.md) — 实验中使用的 TLBI 指令
- [§17.4 BBM](04-bbm.md) — BBM 协议详解
- [§17.5 内核 TLB 维护场景](05-tlb-scenarios.md) — Linux 内核的 TLB 操作
