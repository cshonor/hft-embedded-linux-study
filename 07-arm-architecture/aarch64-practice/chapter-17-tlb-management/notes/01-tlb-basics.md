# §17.1 TLB 基本概念

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

TLB（Translation Lookaside Buffer）是页表项的 cache，缓存 VA→PA 映射。TLB 命中直接得 PA，未命中则 MMU 遍历 4 级页表（最多 4 次内存读）。大页可减少 TLB 条目数。

## 核心要点

### TLB 工作流程

```
CPU 发出 VA
  → TLB 查找
  → 命中：直接得 PA
  → 未命中：MMU walk 4 级页表（慢！4 次内存访问）
  → 填充 TLB
```

### TLB miss 代价

| 情况 | 访存次数 | 估计延迟 |
|------|----------|----------|
| TLB 命中 | 0（TLB 直接返回） | ~1 cycle |
| TLB miss, L1 页表命中 | 1 | ~5 cycle |
| TLB miss, 全 miss | 4（4 级页表） | ~200-400 cycle |

> TLB miss 代价：48 位 VA 4 级页表 = 最多 4 次内存读。
> 大页（2MB/1GB）可减少 TLB 条目数，提高命中率。

### 大页 vs 小页

| 页大小 | 映射 1GB 需要 | TLB 条目数 |
|--------|-------------|-----------|
| 4KB | 262144 个 | 262144 |
| 2MB | 512 个 | 512 |
| 1GB | 1 个 | 1 |

## HFT 关联

TLB miss 是 HFT 延迟抖动的主要来源之一——一次 TLB miss 可能引入 200-400ns 的延迟（4 次内存访问）。HFT 系统应尽量减少 TLB miss：1) 使用大页（2MB 或 1GB）减少 TLB 条目数；2) 预热 TLB（访问所有热数据区域让 TLB 填充）；3) 避免 TLB shootdown（多核间的 TLB 刷新）。Linux 的 `madvise(MADV_HUGEPAGE)` 或 `transparent_hugepage` 可以自动使用大页。

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

## 参考与延伸

- [§17.2 ASID](02-asid.md) — 减少 TLB flush 的机制
- [§17.3 TLB 刷新指令](03-tlb-flush.md) — TLB 维护
- [Ch14 §14.2 四级页表](../../chapter-14-memory-management/notes/section-0-本章完整概述.md) — 页表 walk 的详细过程
