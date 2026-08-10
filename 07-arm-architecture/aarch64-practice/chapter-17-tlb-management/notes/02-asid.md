# §17.2 ASID（Address Space ID）

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ASID 区分不同进程的 TLB 条目，切换进程时只换 ASID 不刷 TLB。本节分析 ASID 机制、TTBR 寄存器中的 ASID 编码、ASID 分配与回收策略、以及 ASID 在 Linux 内核中的实现。

## 核心要点

### ASID 机制

| 特性 | 说明 |
|------|------|
| ASID 宽度 | 8 位或 16 位（TCR_EL1.AS 选择） |
| 作用 | TLB 条目带 ASID 标签，切换进程不刷全部 TLB |
| TCR_EL1.AS | ASID 宽度选择（0=8bit, 1=16bit） |
| TTBRx_EL1 | 高位存放 ASID |
| nG 位 | PTE 中 nG=1 表示该映射属于特定 ASID |

### TTBR0_EL1 中的 ASID 编码

```
TTBR0_EL1:
┌──────────────────────────────────────────────┐
│ ASID   │              BADDR                   │
│ [63:48]│              [47:1]                   │
└──────────────────────────────────────────────┘
  16 bit              47 bit（L0 页表基址）

8 位 ASID 模式（TCR_EL1.AS=0）:
┌──────────────────────────────────────────────┐
│ ASID   │           Reserved           │ BADDR │
│ [63:56]│           [55:48]            │[47:1] │
└──────────────────────────────────────────────┘

ASID 写入示例：
// 8 位模式：ASID 放 bit[63:56]
new_ttbr = (asid << 56) | (l0_table_phys >> 1);
msr TTBR0_EL1, new_ttbr
isb
```

### 有无 ASID 对比

| 场景 | 无 ASID | 有 ASID |
|------|---------|---------|
| 进程切换 | flush 全部 TLB | 只换 ASID |
| TLB 条目 | 无进程标签 | 带 ASID 标签 |
| 切回旧进程 | TLB cold（全 miss） | TLB hot（旧条目仍在） |
| TLB 利用率 | 低（频繁 rebuild） | 高（条目跨切换保留） |
| 性能 | 差 | 好 |

### ASID 分配与回收

```c
// Linux 内核 ASID 分配逻辑（简化）
#define NUM_ASIDS 256  // 8 位 ASID

asid_t alloc_asid(void) {
    asid_t asid;
    // 从 ASID 池中分配一个未使用的 ASID
    asid = find_first_zero_bit(asid_bitmap, NUM_ASIDS);
    if (asid >= NUM_ASIDS) {
        // ASID 耗尽：回收所有 ASID，全刷 TLB
        flush_all_cpu_tlb();
        clear_asid_bitmap();
        asid = 0;
    }
    set_bit(asid, asid_bitmap);
    return asid + 1;  // ASID 0 保留给内核
}

// 16 位 ASID 支持 65536 个进程，几乎不会耗尽
```

### ASID 耗尽处理

| ASID 宽度 | 最大进程数 | 耗尽频率 | 处理方式 |
|-----------|-----------|---------|---------|
| 8 位 | 255 | 常见（服务器） | 回收所有 ASID + 全刷 TLB |
| 16 位 | 65535 | 极少 | 同上，但几乎不触发 |

> ASID 耗尽时全刷 TLB 会导致短暂性能下降，但 16 位 ASID 几乎不会耗尽。

### 设置 ASID

```asm
// 设置 ASID（进程切换时）
// 8 位模式：ASID 放 TTBR0[63:56]
msr TTBR0_EL1, x0      // x0 高位包含 ASID
isb

// 16 位模式：ASID 放 TTBR0[63:48]
// TCR_EL1.AS=1 启用 16 位 ASID
msr TCR_EL1, x1        // x1.AS = 1
isb
msr TTBR0_EL1, x0      // x0[63:48] = ASID
isb

// 带 ASID 的 TLB 刷新（只刷当前 ASID）
tlbi aside1, x0        // x0 = ASID
dsb sy
isb
```

### PTE nG 位与 ASID 关系

```
PTE[11] = nG（non-Global）

nG = 0（全局映射）：
  → TLB 条目不带 ASID 标签
  → 所有进程共享（如内核代码）
  → 进程切换不失效

nG = 1（非全局映射）：
  → TLB 条目带 ASID 标签
  → 只属于该进程（如用户空间）
  → 进程切换时保留（靠 ASID 区分）
```

| nG 值 | 映射类型 | TLB 标签 | 适用场景 |
|-------|---------|---------|---------|
| 0 | 全局 | 无 ASID | 内核代码/数据 |
| 1 | 非全局 | 有 ASID | 用户空间映射 |

> **没有 ASID**：每次进程切换都要 flush 全部 TLB → 性能差。
> **有 ASID**：切换进程时只换 ASID，旧进程的 TLB 条目仍在（下次切回来命中）。

## HFT 关联

HFT 系统通常是单进程裸金属，不涉及进程切换，ASID 的价值不大。但如果 HFT 系统有管理进程（如监控进程），ASID 可以避免管理进程和交易进程的 TLB 互相 flush。

### Linux HFT 中的 ASID

在 Linux HFT 方案中，ASID 是默认启用的——Linux 进程切换时设置 ASID，TLB 条目跨切换保留，减少 TLB rebuild 开销。ASID 8 位只能区分 256 个进程，16 位可以 65536 个，现代 ARM 支持 16 位 ASID。

### 检测 ASID 支持

```c
// 检查是否支持 16 位 ASID
uint64_t tcr;
asm volatile("mrs %0, TCR_EL1" : "=r"(tcr));
bool asid_16bit = (tcr >> 36) & 1;  // TCR_EL1.AS

// 检查 ID_AA64MMFR0_EL1 中的 ASIDBits 字段
uint64_t mmfr0;
asm volatile("mrs %0, ID_AA64MMFR0_EL1" : "=r"(mmfr0));
int asid_bits = (mmfr0 >> 4) & 0xf;  // 0=8bit, 2=16bit
```

## 自测题

1. **ASID 解决什么问题？有了 ASID 后进程切换还需要刷全部 TLB 吗？**

<details>
<summary>答案</summary>

ASID 解决**进程切换时 TLB flush 的性能问题**。没有 ASID 时，每次进程切换必须 flush 全部 TLB（因为新旧进程的 VA 相同但映射不同）。有 ASID 后，TLB 条目带 ASID 标签，切换进程只换 ASID，**不需要刷 TLB**。旧进程的 TLB 条目仍在，下次切回来直接命中。
</details>

2. **ASID 存放在哪里？8 位 ASID 最多支持多少进程？**

<details>
<summary>答案</summary>

ASID 存放在 **TTBR0_EL1 的高位**（bit[63:48] 或 bit[63:56]，取决于 AS 宽度）。8 位 ASID 最多支持 **256** 个进程。16 位 ASID（TCR_EL1.AS=1）支持 65536 个。如果进程数超过 ASID 上限，内核需要回收旧 ASID 并 flush 对应 TLB。
</details>

3. **`tlbi aside1, x0` 和 `tlbi alle1` 的区别？**

<details>
<summary>答案</summary>

- `tlbi aside1, x0`：只刷**指定 ASID** 的 TLB 条目（x0 = ASID），其他 ASID 的条目保留
- `tlbi alle1`：刷**所有 ASID** 的 EL1 TLB 条目，全部清空

`aside1` 精确、影响小；`alle1` 粗暴、影响大。进程退出时用 `aside1` 只刷该进程的 TLB。
</details>

4. **PTE 中 nG 位的作用是什么？nG=0 和 nG=1 有什么区别？**

<details>
<summary>答案</summary>

nG（non-Global）位决定 TLB 条目是否带 ASID 标签：
- **nG=0**（全局映射）：TLB 条目不带 ASID，所有进程共享。适用于内核代码/数据，进程切换不影响。
- **nG=1**（非全局映射）：TLB 条目带 ASID 标签，只属于该进程。适用于用户空间映射，进程切换时靠 ASID 区分。
</details>

## 参考与延伸

- [§17.1 TLB 基本概念](01-tlb-basics.md) — TLB 命中/未命中
- [§17.3 TLB 刷新指令](03-tlb-flush.md) — aside1/alle1 等指令
- [§17.5 内核 TLB 维护场景](05-tlb-scenarios.md) — 进程切换的 TLB 操作
