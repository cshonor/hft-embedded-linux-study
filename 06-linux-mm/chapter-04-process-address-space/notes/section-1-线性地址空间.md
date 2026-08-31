# Ch 4 §1 线性地址空间（x86_64 / ARM64 布局全图）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`Documentation/arch/x86/x86_64/mm.rst`、`arch/arm64/include/asm/memory.h`）

---

## 本节讲什么

地址空间怎么切、各段放什么、mmap 从哪分配——这是读 `/proc/maps`、排查"为什么 hugepage 映射在这个地址"、做 ASLR/固定布局决策的地图课。原书是 32 位三段模型；v6.6 的 64 位布局复杂得多，逐段讲。

---

## 1. 从原书模型到 64 位

```
x86-32（原书）：                       x86_64（v6.6，48-bit VA）：
┌───────────────┐ 0xFFFFFFFF         ┌───────────────────────┐ 0xFFFF8000_00000000
│ 内核 1GiB     │                    │ 内核空间（canonical 高半）│
├───────────────┤ 0xC0000000         │  ├ fixmap/vmalloc      │
│ 用户 3GiB     │                    │  ├ vmemmap             │
│               │                    │  └ linear map          │
└───────────────┘ 0x00000000         ├───────────────────────┤ 0x00007FFF_FFFF0000
                                      │ 用户空间（栈顶之下）    │
                                      │  ├ stack（ASLR）       │
                                      │  ├ mmap 区（自上而下）  │
                                      │  ├ heap（brk，自下而上）│
                                      │  ├ .text/.data（PIE）  │
                                      └───────────────────────┘ 0x00000000
```

| 概念 | 原书 32 位 | x86_64 v6.6 | ARM64（4K/48VA） |
|------|-----------|-------------|-------------------|
| 用户上限 TASK_SIZE | 3GiB | 128TiB | 256GiB(39VA)~128TiB(48VA) 按 CONFIG |
| 内核起点 | 0xC0000000 | 0xFFFF8000... | 0xFFFF8000...（TTBR1） |
| 用户 mmap 基址 | 固定附近 | `mmap_base`（ASLR 随机）+ **自顶向下** | 同左 |
| 内核/用户切换 | 同一张表不同段 | **双表**（KPTI）/ TTBR0/TTBR1 | 同左 |

## 2. 内核半边的关键区（x86_64）

| 区 | 起点（v6.6 典型） | 用途 | 前文对应 |
|----|--------------------|------|----------|
| LDT/user 模式区 | …00000000 | 与用户共享的入口跳板（KPTI） | Ch3 §4 |
| vmalloc/ioremap | …f0000000 上方起 | 动态内核映射 | Ch3 §5 |
| vmemmap | …fd000000? → 0xffffea00… | `struct page` 连续区（SPARSEMEM_VMEMMAP） | Ch3 §5 |
| cpu_entry_area | 每核固定 | 入口栈/DSB（Spectre 缓解） | — |
| linear map（直映） | 0xffff8880… | 全部物理内存平移映射 | Ch3 §5 |
| 模块区 | 0xffffffffa00… | 内核模块（与内核 .text 邻近） | — |

**要点：** 内核 VA 不是"一段"，是 **多个功能子区**——写内核代码时从 `virt_to_page`/`vmalloc_to_page` 的合法性差异就能反推出你落在哪个区（Ch3 §5 的 API 表）。

## 3. 用户半边的分配方向（v6.6）

```
mmap_base（ASLR 后）
    ↓ mmap 自顶向下分配（地址递减）
    ...
    ↑ brk heap 自底向上（从 exe .data 之后 + 随机 gap）
    .text/.data（PIE 则也随机）
```

| 事实 | HFT 意义 |
|------|----------|
| mmap 从高往低 | 连续 mmap 地址递减；固定地址策略（`MAP_FIXED_NOREPLACE`）需自己规划 |
| `mmap_min_addr` | 低地址禁止映射（防 null deref 利用）——想 map 到 0 做某些硬件 trick 会被拒 |
| 栈上限 `RLIMIT_STACK` | 深递归策略代码注意；栈 VMA 自动增长向下 |
| THP 的对齐 | 2MiB 大页要求 VMA 地址/长度 2MiB 对齐——`posix_memalign`/手动 mmap hint |
| ASLR | 生产引擎要 **关闭自身 ASLR**（布局可比对、故障可复现）：`setarch -R` |

**x86_64 5 级分页（la57）下用户上限 128PiB**，但 mmap 默认仍从老 47-bit 顶部往下分——兼容性优先。

## 4. ARM64 特有：TTBR0/TTBR1 天然分界

ARM64 用户态走 TTBR0、内核走 TTBR1 两套页表基址——**硬件级分界**，上下文切换只换 TTBR0。对比 x86 的单 CR3 + KPTI 软件模拟，ARM64 免了切换税（Ch3 §4 的 HFT 成本讨论在此兑现）。

## 5. 观测

```bash
cat /proc/self/maps                      # 本进程 VMA 布局（下节主角）
cat /proc/self/smaps | grep -A2 '\[stack\]'
pmap -x <pid>                            # RSS/Dirty 分解
cat /proc/vmallocinfo                    # 内核半边的 vmalloc 占用（谁用了多少）
```

**排障例：** 引擎 hugepage 映射没落在预期地址 → 查 maps 看是否被 PIE/ASLR 顶掉、或与 glibc arena 冲突——`MAP_FIXED_NOREPLACE` 探测式放置。

## 6. HFT / 嵌入式关联

| 主题 | 动作 |
|------|------|
| 布局确定性 | 启动时 `MAP_FIXED_NOREPLACE` 定格所有区域；关 ASLR |
| 大页对齐 | mmap hint 按 2MiB 对齐；检查 maps 里的对齐实况 |
| 内核半边泄漏排查 | `/proc/vmallocinfo` 按调用者聚合（驱动泄漏常客） |
| 树莓派 | 1GiB 内存 + CMA 预留 → 用户 mmap 区照样 39-bit 上限不受影响，但 free 页紧张提前 |

## 7. 衔接

- [§2 mm_struct](./section-2-进程地址空间描述符.md)：这个空间的"户口本"
- [Ch 3 §4/§5](../../chapter-03-page-table-management/)：内核半边各区的页表机制
- [06.5/ch05 maple tree](../../../06.5-modern-mm/chapter-05-vm-address-space-maple-tree/)：VMA 组织的现代形态

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 mmap 区自顶向下而不是从低处向上长？**
A：历史布局让 heap（brk）向上、mmap 向下，中间留白给两边增长——32 位时代防止两者过早相遇。64 位地址过剩，此理由消失但方向保留（兼容+栈在顶部的天然邻居）。改 `sysctl vm.legacy_va_layout` 可回老行为（别在生产动）。

**Q2：`MAP_FIXED_NOREPLACE` 和老 `MAP_FIXED` 差在哪？**
A：NOREPLACE 若目标区间已有映射则 **失败**（EEXIST），绝不覆盖；老 MAP_FIXED 直接拆掉已有映射（把 glibc 的 heap 拆了都不知道）。新代码一律用 NOREPLACE + 探测循环。

**Q3：`/proc/maps` 里 vdso/vvar 是什么？**
A：vDSO——内核映射进用户态的"微型系统调用库"（clock_gettime/gettimeofday 不进内核直接读），vvar 是其数据页。HFT 计时热路径的关键：**vdso 的 clock_gettime(CLOCK_MONOTONIC) ≈ 20ns，真 syscall ≈ 100-200ns**。详见 §6。

**Q4：KPTI 对用户半边布局有影响吗？**
A：没有（用户区本来就在自己的表里）；影响的是 **内核半边被复制进用户表的最小 trampoline**——即 cpu_entry_area 那一小块"用户可见的内核区"。这是 x86 专有税，ARM64 无此问题。

**Q5：怎么确认一段映射真的用了 2MiB 大页？**
A：`/proc/pid/smaps` 该 VMA 的 `AnonHugePages`/`KernelPageSize: 2048 kB`（hugetlb 映射）；或 `pagemap` 看 PTE 的PFN 连续性。只看 maps 的地址对齐不可靠——对齐是必要条件非充分。

</details>

---
