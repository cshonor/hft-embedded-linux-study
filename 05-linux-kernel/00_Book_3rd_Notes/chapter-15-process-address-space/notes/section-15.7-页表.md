## ⑦ 页表 · Page Tables

程序使用 **虚拟地址（VA）**；CPU 与 DMA（经 IOMMU）访问 **物理地址（PA）** — **页表** 保存 **VA→PFN** 映射与 **R/W/X** 权限。

#### 多级页表（书中三级模型）

| 级 | 名称 | 作用 |
|----|------|------|
| 1 | **PGD（页全局目录）** | `mm_struct->pgd` — **最高级** |
| 2 | **PMD（页中间目录）** | 中间索引 |
| 3 | **PTE（页表项）** | **PFN + 权限位** |

```
VA 分解（概念）:
  [ PGD index | PMD index | PTE index | page offset ]

MMU walk:
  VA ──► PGD ──► PMD ──► PTE ──► 物理页帧 (PFN) + offset
```

#### x86-64 四级（书外补全）

| 级 | 名称 |
|----|------|
| + | **PUD（页上级目录）** |
| 52-bit VA | **稀疏** — 未用区间 **无中间表** |

| 事实 | 含义 |
|------|------|
| **大页 PTE** | **2MB / 1GB** — **一级更少 walk** |
| **PCID** | **Process Context ID** — **切换 mm 少 flush TLB** |

#### PTE 权限位（概念）

| 位 | 含义 |
|----|------|
| **Present** | 在内存 — 否则 **#PF** |
| **RW** | 可写 |
| **User/Supervisor** | 用户态能否访问 |
| **NX / XD** | **不可执行** — **DEP** |
| **Dirty / Accessed** | 写/读 **过** — 回收参考 |

#### TLB · Translation Lookaside Buffer

| 硬件 | **片上缓存** 最近 **VA→PA** |
|------|---------------------------|
| **TLB hit** | **~1 cycle** 量级 — 正常访问 |
| **TLB miss** | **页表 walk** — **数十～数百 cycle** |
| **切换 `mm`** | **`write CR3`** / **PCID** — **invalidate** 部分条目 |
| **大页** | **同样 2MB 范围占 1 TLB 项** vs 512 个 4KB 项 |

```
访问 VA
    ├─ TLB hit  ──► PA（快）
    └─ TLB miss ──► 页表 walk ──► 填 TLB ──► PA（慢）
```

#### 透明大页 · THP（现代补充）

| 模式 | 行为 |
|------|------|
| **THP** | 内核 **自动**  promoted 4KB → **2MB** |
| **HFT 倾向** | **关闭 THP** 或 **always never** — **合并/分裂** 引入 **latency 抖动** |
| **显式 hugetlb** | **`MAP_HUGETLB`** — **确定性** |

#### 内核 vs 用户页表

| | 用户 VA | 内核 VA |
|--|---------|---------|
| **切换** | 随 **`mm_struct`** | **共享 upper half**（32 位）或 **独立 direct map**（64 位） |
| **fault** | **`handle_mm_fault`** | **内核 bug** 若缺页 |

**HFT：** **TLB miss 是隐形杀手** — **`perf stat -e dTLB-load-misses`** 对比 **4KB vs 2MB**。绑核减 **跨核 TLB shootdown**；**1GB 页** 适合 **巨型 **只读** 参考数据**（慎用 **分裂成本**）。

→ [01 CSAPP Ch9 TLB/翻译](../../../../02-computer-systems/chapter-09-virtual-memory/notes/section-9.6-9.7-地址翻译与Linux案例.md) · [06 Gorman Ch3 页表](../../../../06-linux-mm/chapter-03-page-table-management/) · [06 THP note](../../../../06-linux-mm/chapter-03-page-table-management/notes/note-透明大页THP.md)


> ↔ [ULK Ch9 §4 缺页异常](../../../../20-linux-kernel-deep/chapter-09-process-address-space/notes/section-4-缺页异常.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** x86_64 四级页表的翻译过程？TLB miss 的代价是多少？

<details><summary>答案</summary>

4 级页表：CR3 → PML4 → PDPT → PD → PT → 物理页。48 位 VA = 9+9+9+9+12。TLB miss 时 CPU 硬件遍历 4 级页表（4 次内存访问），约 100-300ns。TLB hit 约 1-2ns。Huge Page（2MB）只需 3 级页表，减少一级查找 + 大幅减少 TLB 条目数。HFT 用 Huge Page 将 TLB miss 率从 5% 降到 < 0.1%。

</details>

**Q2.** PTE 的 Dirty 位和 Accessed 位分别什么作用？

<details><summary>答案</summary>

Dirty 位：页被写过（写回磁盘时需要 flush）。Accessed 位：页被读过或写过（页面回收时优先换出未访问页）。这两个位由 CPU 硬件设置，内核清除。页面回收器扫描 Accessed 位判断页面活跃度。HFT 内存锁定后不参与回收，Accessed 位不重要。

</details>

</details>
---
