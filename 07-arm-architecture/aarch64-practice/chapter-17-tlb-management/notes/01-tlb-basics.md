# §17.1 TLB 基本概念

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

TLB（Translation Lookaside Buffer）是页表项的 cache，缓存 VA→PA 映射。TLB 命中直接得 PA，未命中则 MMU 遍历 4 级页表（最多 4 次内存读）。本节分析 TLB 工作原理、miss 代价、大页优势以及 HFT 场景下的优化策略。

## 核心要点

### TLB 工作流程

```
CPU 发出 VA
  → TLB 查找（按 ASID + VA index 查）
  → 命中：直接得 PA（~1 cycle）
  → 未命中：MMU walk 4 级页表（慢！4 次内存访问）
    → L0 页表项 → L1 页表项 → L2 页表项 → L3 页表项
    → 得到 PA，填充 TLB
  → 用 PA 访问 cache/内存
```

### TLB 硬件结构

| 层次 | 容量（典型） | 命中延迟 | 说明 |
|------|-------------|---------|------|
| L1 D-TLB | 48-64 条目 | 1 cycle | 数据 TLB，每核私有 |
| L1 I-TLB | 32-48 条目 | 1 cycle | 指令 TLB，每核私有 |
| L2 TLB（STLB） | 1024-2048 条目 | 5-7 cycle | 共享 TLB，统一 I/D |
| Page Walk | — | 200-400 cycle | 4 级页表遍历 |

### TLB miss 代价

| 情况 | 访存次数 | 估计延迟 | 说明 |
|------|----------|----------|------|
| TLB 命中 | 0（TLB 直接返回） | ~1 cycle | 最佳情况 |
| TLB miss, STLB 命中 | 0（L2 TLB 返回） | ~5-7 cycle | 次佳 |
| TLB miss, L0 页表命中 | 1 | ~5 cycle | 页表在 L1 cache |
| TLB miss, 全 miss | 4（4 级页表） | ~200-400 cycle | 最差情况 |

> TLB miss 代价：48 位 VA 4 级页表 = 最多 4 次内存读。
> 大页（2MB/1GB）可减少 TLB 条目数，提高命中率。

### 4 级页表 walk 详解

```
VA = [L0_index(9bit) | L1_index(9bit) | L2_index(9bit) | L3_index(9bit) | offset(12bit)]

Step 1: 从 TTBR0_EL1 取 L0 表基址 + L0_index × 8 → L0 页表项
Step 2: L0 表项指向 L1 表基址 + L1_index × 8 → L1 页表项
Step 3: L1 表项指向 L2 表基址 + L2_index × 8 → L2 页表项
Step 4: L2 表项指向 L3 表基址 + L3_index × 8 → L3 页表项 → 得到 PA

如果 L1/L2 是 Block descriptor（大页映射），提前终止：
  L1 Block → 1GB 映射（2 次访存）
  L2 Block → 2MB 映射（3 次访存）
```

### 大页 vs 小页

| 页大小 | 映射 1GB 需要 | TLB 条目数 | walk 次数 |
|--------|-------------|-----------|----------|
| 4KB | 262144 个 | 262144 | 4 次 |
| 2MB | 512 个 | 512 | 3 次 |
| 1GB | 1 个 | 1 | 2 次 |

### TLB 条目结构

```
┌─────────────────────────────────────────────┐
│ ASID(8/16bit) │ VA Tag │ PA │ Attr │ Valid  │
└─────────────────────────────────────────────┘
  │              │        │    │       │
  │              │        │    │       └─ 有效位
  │              │        │    └─ 页属性（MAIR/AP/UXN 等）
  │              │        └─ 物理地址
  │              └─ 虚拟地址标签
  └─ Address Space ID（区分不同进程）
```

### ARMv8 TLB 特性

| 特性 | 说明 | 寄存器/指令 |
|------|------|------------|
| ASID 支持 | TLB 条目带 ASID 标签 | TCR_EL1.AS |
| 多种 flush 粒度 | 全刷/按 ASID/按 VA | TLBI 指令 |
| Inner Shareable flush | 跨核 TLB 刷新 | TLBI ...is |
| nG 位 | 非全局映射（带 ASID） | PTE.nG |
| 大页支持 | 2MB/1GB Block | 页表 descriptor type |

## HFT 关联

TLB miss 是 HFT 延迟抖动的主要来源之一——一次 TLB miss 可能引入 200-400ns 的延迟（4 次内存访问）。

### HFT TLB 优化策略

```c
// 策略1：使用大页（2MB）
void *buf = mmap(NULL, 2*1024*1024, PROT_READ|PROT_WRITE,
                 MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB, -1, 0);

// 策略2：透明大页
madvise(buf, size, MADV_HUGEPAGE);

// 策略3：TLB 预热（启动时访问所有热数据）
void warmup_tlb(void *base, size_t size) {
    char *p = (char *)base;
    for (size_t off = 0; off < size; off += PAGE_SIZE_2M) {
        asm volatile("ldr x0, [%0]" :: "r"(p + off));
    }
}

// 策略4：避免 TLB shootdown
// - 启动后不修改页表
// - 不调用 mprotect/munmap
// - 线程绑定 CPU 避免进程切换
```

HFT 系统应尽量减少 TLB miss：1) 使用大页（2MB 或 1GB）减少 TLB 条目数；2) 预热 TLB（访问所有热数据区域让 TLB 填充）；3) 避免 TLB shootdown（多核间的 TLB 刷新）。Linux 的 `madvise(MADV_HUGEPAGE)` 或 `transparent_hugepage` 可以自动使用大页。

## 自测题

1. **TLB miss 在 4 级页表下最多需要几次内存访问？**

<details>
<summary>答案</summary>

最多 **4 次**内存访问（L0 → L1 → L2 → L3 页表）。每次访问可能是 cache miss（~100ns/次），最坏情况约 400ns。这是 TLB miss 的代价——也是 HFT 系统需要大页的原因。
</details>

2. **映射 1GB 内存，用 4KB 页和 2MB 页分别需要多少 TLB 条目？**

<details>
<summary>答案</summary>

- 4KB 页：1GB / 4KB = **262144 个** TLB 条目（远超 TLB 容量，大量 miss）
- 2MB 页：1GB / 2MB = **512 个** TLB 条目（可能仍在 TLB 中）
- 1GB 页：1GB / 1GB = **1 个** TLB 条目（必定命中）

大页大幅减少 TLB 压力。
</details>

3. **HFT 系统如何减少 TLB miss？**

<details>
<summary>答案</summary>

1. **使用大页**（2MB/1GB）：减少 TLB 条目数
2. **TLB 预热**：启动时访问所有热数据区域，填充 TLB
3. **避免 TLB shootdown**：减少多核间的页表修改
4. **减少进程切换**：每次切换可能 flush TLB（无 ASID 时）
5. **使用 ASID**：进程切换不 flush TLB
6. **Pin 内存**：避免换页导致的 TLB 失效
</details>

4. **TLB 条目中为什么需要 ASID 字段？**

<details>
<summary>答案</summary>

ASID 区分不同进程的 TLB 条目。不同进程的相同 VA 映射到不同 PA。没有 ASID 时，每次进程切换必须 flush 全部 TLB（因为无法区分条目属于哪个进程）。有 ASID 后，TLB 条目带进程标签，切换时只换 ASID，旧进程的 TLB 条目保留——下次切回来直接命中，避免 TLB rebuild 开销。
</details>

## 参考与延伸

- [§17.2 ASID](02-asid.md) — 减少 TLB flush 的机制
- [§17.3 TLB 刷新指令](03-tlb-flush.md) — TLB 维护
- [Ch14 §14.2 四级页表](../../chapter-14-memory-management/notes/section-0-本章完整概述.md) — 页表 walk 的详细过程
