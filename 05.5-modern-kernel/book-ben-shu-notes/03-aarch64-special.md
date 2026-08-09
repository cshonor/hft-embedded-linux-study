# ARM64 专题

> 笨叔《奔跑吧 Linux 内核》读书笔记
> 对应旧书: ULK3 / LKD3 (Linux 2.6)
> 对应现代内核: Linux 5.x / 6.x

---

## 本节要点

ARM64（AArch64）在 HFT 和嵌入式领域的重要性持续增长。现代内核对 ARM64 的支持已与 x86_64 平起平坐，但架构差异带来独特的内核行为：

- **ARM64 是弱排序架构**：load/store 可能被 CPU 重排，内核代码需要正确的内存屏障（`dmb`/`dsb`/`isb`），而 x86 的 TSO 模型更宽松
- **TLB 管理不同**：ARM64 用 TLBI 指令失效 TLB 项，x86 用 INVLPG。ARM64 的 TLB 失效更昂贵（需要 DSB 同步）
- **页表格式**：ARM64 支持 4K/16K/64K 页大小 + 3 级或 4 级页表，x86_64 固定 4K + 4 级
- **GIC（Generic Interrupt Controller）**：ARM64 中断控制器与 x86 的 APIC 完全不同，影响中断延迟和亲和性设置
- **virtual timer vs physical timer**：ARM64 有独立的虚拟定时器（EL1），减少 hypervisor 介入
- **SVE/Scalable Vector Extension**：ARM64 独有，HFT 可用于向量化风控计算

---

## 与旧书对比

| ULK3 / LKD3 (2.6) | 笨叔 (5.x/6.x) | 变化原因 |
|--------------------|-----------------|----------|
| 主要 x86 视角 | ARM64 一等公民支持 | 服务器/HFT 采用 ARM64 |
| x86 TSO 内存模型 | ARM64 弱排序 + dmb/dsb | 架构差异 |
| APIC 中断控制器 | GIC v3/v4（ITS） | ARM 中断架构 |
| INVLPG 单页 TLB 失效 | TLBI + DSB（更昂贵） | ARM TLB 硬件设计 |
| 固定 4K 页 | 4K/16K/64K 可配置 | ARM64 页大小灵活 |
| x86 rdtsc 计时 | ARM64 cntvct_el0 | 不同硬件计数器 |
| x86 IPI（APIC IPI） | ARM64 SGI（GIC 软中断） | IPI 机制不同 |

---

## 关键数据结构 / 函数

```
// 源码路径: arch/arm64/include/asm/
//          arch/arm64/kernel/
//          arch/arm64/mm/

// 页表项（PTE）格式
typedef struct { pteval_t pte; } pte_t;
// ARM64 PTE 位: 
//   bit[1]  - Page table/bit（block vs page）
//   bit[6]  - AP[1]（access permission: user access）
//   bit[7]  - AP[2]（read-only）
//   bit[10] - AF（access flag, 类似 x86 Accessed bit）
//   bit[11] - nG（non-global, 类似 x86 PCID）

// 内存屏障
#define dmb(opt)  asm volatile("dmb " #opt ::: "memory")  // 数据内存屏障
#define dsb(opt)  asm volatile("dsb " #opt ::: "memory")  // 数据同步屏障（更强）
#define isb()     asm volatile("isb" ::: "memory")        // 指令同步屏障

// 中断控制器
struct gic_chip_data {
    struct irq_chip chip;
    void __iomem *dist_base;     // GIC Distributor
    void __iomem *cpu_base;      // GIC CPU interface
    // GICv3 用 ITS（Interrupt Translation Service）
};

// 定时器
// ARM64 虚拟定时器寄存器
//   CNTV_CTL_EL0  - 控制寄存器（enable/mask）
//   CNTV_CVAL_EL0 - 比较值（64 位）
//   CNTVCT_EL0    - 当前计数（类似 x86 TSC）

// 系统寄存器访问
#define read_sysreg(reg) ({
    u64 val;
    asm volatile("mrs %0, " #reg : "=r"(val));
    val;
})
#define write_sysreg(val, reg) ({
    u64 __val = (val);
    asm volatile("msr " #reg ", %0" :: "r"(__val));
})
```

---

## HFT 关联

ARM64 在 HFT 领域的应用（如 AWS Graviton、Ampere Altra）需要特别注意：

1. **内存屏障开销**：ARM64 的 `dmb ish`（inner shareable）比 x86 的 `mfence` 更常被需要——因为弱排序模型下无锁代码需要显式屏障。`smp_store_release` / `smp_load_acquire`（ARM64 用 `stlr` / `ldar` 指令）是比手动 `dmb` 更高效的替代
2. **TLB 失效更昂贵**：ARM64 TLBI 需要 DSB 同步，多核 TLB shootdown 比 x86 慢。HFT 用 Huge Page 减少 TLB miss，同时避免 mmap/munmap 在热路径（触发 TLB shootdown）
3. **中断延迟**：GIC v3 的 ITS 支持中断直通（LPI → 设备 → CPU），减少中断分发延迟。HFT 配置 `irq affinity` 绑定网卡中断到交易核
4. **页大小选择**：16K 或 64K 页减少 TLB miss，但可能增加内部碎片。HFT 建议用 2MB Huge Page（4K 基页）或 64K 基页
5. **CPU 计时**：`cntvct_el0` 读取虚拟定时器计数，精度与 TSC 类似但需要 `isb` 序列化。HFT 计时用 `clock_gettime(CLOCK_MONOTONIC_RAW)` 或直接读 `cntvct_el0`

**建议**：HFT 移植 x86 → ARM64 时重点检查：1) 无锁代码的内存屏障；2) TLB shootdown 频率；3) 中断延迟测量；4) rdtsc → cntvct 替换。

---

## 自测

<details>
<summary>Q1: ARM64 的弱排序内存模型对 HFT 无锁代码有什么影响？</summary>

ARM64 CPU 可以重排 load/store 指令以提高流水线效率。例如 store A=1; store B=2; 另一个 CPU 可能先看到 B=2 再看到 A=1。HFT 无锁代码（如 ring buffer 的 producer/consumer）必须用内存屏障保证可见性顺序。`smp_store_release(&ready, 1)` 确保 ready=1 之前的所有 store 对其他 CPU 可见后才设置 ready；`smp_load_acquire(&ready)` 确保 ready 读取后的所有 load 在 ready 之后的地址执行。ARM64 上 release/acquire 编译为 `stlr`/`ldar` 指令，比 `dmb ish` 更轻量。x86 的 TSO 模型天然保证 store-store 和 load-load 不重排，所以 x86 上 `smp_store_release` 几乎无开销——跨架构移植时不能假设 x86 行为。

</details>

<details>
<summary>Q2: ARM64 的 TLB 失效为什么比 x86 更昂贵？对 HFT 有什么影响？</summary>

x86 用 `INVLPG addr` 单步失效一个 TLB 项，硬件自动处理。ARM64 用 `TLBI VAALE1IS, addr` 失效 TLB 项，但需要 `DSB ISH`（Data Synchronization Barrier, Inner Shareable）等待所有 CPU 确认失效完成——这个同步开销在多核系统上显著。多核 TLB shootdown（如 munmap 触发）在 ARM64 上比 x86 慢 2-5 倍。HFT 影响：热路径避免 mmap/munmap（触发 TLB shootdown）；用 Huge Page 减少 TLB 条目数（2MB 页覆盖 512 个 4K 页，TLB 条目减少 512 倍）；mlock 锁定页面防止被回收（回收也涉及 TLB 失效）。

</details>

<details>
<summary>Q3: HFT 系统从 x86 迁移到 ARM64 需要检查哪些关键点？</summary>

1) **内存屏障**：所有无锁代码（ring buffer/SPSC 队列/序列锁）必须用 `smp_store_release`/`smp_load_acquire`，不能用 x86 的 bare `WRITE_ONCE`/`READ_ONCE`（x86 TSO 下可以，ARM64 弱排序下会出 bug）；2) **TLB 行为**：减少 mmap/munmap 频率，评估 Huge Page 配置；3) **中断延迟**：GIC v3 ITS 中断分发路径与 APIC 不同，需重新测量网卡中断→用户态的延迟；4) **计时器**：`rdtsc` → `cntvct_el0`，注意虚拟化环境下虚拟定时器可能被 hypervisor 截获；5) **cache line 大小**：ARM64 通常 64 字节（同 x86），但 Ampere Altra 等可能有差异，false sharing 防护需验证；6) **NUMA 拓扑**：ARM64 服务器 NUMA 行为与 x86 不同，网卡亲和性需重新配置。

</details>
