# Ch 3 §5 地址与 struct page 的映射

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`include/linux/mmzone.h`、`include/asm-generic/memory_model.h`）

---

## 本节讲什么

内核里有 **三套身份** 互相转换：**VA（虚拟地址）、PA（物理地址）、`struct page`（元数据）**。本节讲清转换表——这是读 MM 代码的第二套"识字课"，也是 `virt_to_page` 什么时候 **非法** 的判据来源。

---

## 1. 三套身份一张图

```
      ┌─────────── linear map（直映区） ───────────┐
VA ──►│ PAGE_OFFSET + PA（加减互逆：__va/__pa）    │◄── PA
      └────────────────────────────────────────────┘
PA ──右移──► PFN ──索引──► vmemmap[PFN] == struct page   （pfn_to_page/page_to_pfn）
VA ──virt_to_page──► struct page        （仅直映区合法！）
VA ──页表 walk──► PA/PFN                 （任意映射区）
```

**核心分工：** 页表管 "VA→PA"（per-mm）；vmemmap 管 "PFN→元数据"（全局唯一）。两套系统在 `vm_normal_page()`（memory.c:581）汇合：walk 出 PFN 再查 struct page。

## 2. 直映区（linear/direct map）

| 项 | x86_64 | ARM64（4K 页, 48VA） |
|----|--------|----------------------|
| 起点 | `PAGE_OFFSET = 0xffff888000000000` | `PAGE_OFFSET = 0xffff800000000000` |
| 覆盖 | 全部物理内存线性平移 | 同左（块映射） |
| 转换宏 | `__va(pa)` / `__pa(va)` = ±PAGE_OFFSET | 同左 |
| 典型用户 | slab 对象、buddy 页、`page_address()` | 同左 |

**规则：`virt_to_page()`/`phys_to_virt()` 只对直映区 VA 合法。** 对 vmalloc 地址、用户地址、模块地址调用 = **未定义行为**（地址不在直映窗口，减出来的"PFN"是垃圾）。v6.6 里这些错误调用有的被 `DEBUG_VIRTUAL`（`CONFIG_DEBUG_VIRTUAL`）当场捕获——树莓派调内核时值得开。

## 3. `mem_map[]` → sparse vmemmap

原书时代：全局 `struct page mem_map[]` 一个大数组，PFN 直接下标。

v6.6 两条实现（`CONFIG_SPARSEMEM_*`）：

| 模式 | struct page 存哪 | 特点 |
|------|------------------|------|
| `SPARSEMEM`（classic） | section 表按需分配 | 内存空洞不浪费元数据 |
| `SPARSEMEM_VMEMMAP`（x86_64/ARM64 默认） | vmemmap 虚拟区（x86: `0xffffea0000000000`）**连续**排列 | `pfn_to_page(pfn) = vmemmap + pfn` **一次加法**（O(1)）；空洞 section 的表页不分配但地址空间保留连续假象 |

```c
/* asm-generic/memory_model.h（vmemmap 模式）*/
#define __pfn_to_page(pfn)  (vmemmap + (pfn))
#define __page_to_pfn(page) (unsigned long)((page) - vmemmap)
```

**代价**：vmemmap 区本身要页表映射物理内存——每 4GiB 物理内存的 `struct page`（64B×2^20×... ≈ 每 4KiB 页 64B 元数据 = **1.5% 物理内存**）+ 其页表。512GiB 机器 ≈ 8GiB 元数据。这就是 §4 说的"页表吃内存"的可量化部分。

**HFT 关联**：`pfn_to_page` O(1) 意味着 GUP/回收路径没有隐藏查找成本；但 **元数据 cache miss** 是真实成本——随机访问 4KiB 页时，struct page（64B，同 cache line 挤 1 个）与数据页本身是两次独立 miss。per-CPU page list 顺带把热的 struct page 留在 cache。

## 4. 带洞物理布局：PFN 的"虚号"

PFN 是 **架构物理地址右移**，不是紧凑编号。所以：

- ACPI/PCIe MMIO 洞（x86 3~4GiB）区间的 PFN **没有** struct page
- NUMA 黑洞、内存热插拔空洞同理
- `pfn_valid(pfn)` 必查——直接 `pfn_to_page` 野 PFN 再解引用 = crash。树莓派外设 VA 用 `ioremap` 而非直映，就是为了绕开"无 struct page"区。

## 5. 不经 struct page 的映射

| 映射 | 为什么没有 struct page | PTE 标记 |
|------|------------------------|----------|
| `ioremap()`（MMIO 寄存器） | 设备内存不是 RAM | `_PAGE_PCD/PWT`（不可缓存）|
| `remap_pfn_range()`（用户 mmap 设备） | 同上 | `_PAGE_SPECIAL` |
| frame buffer / DPDK 的 VFIO 映射 | 外部接管的物理页 | VM_IO/VM_PFNMAP 标记 VMA |

`vm_normal_page()`（memory.c:581）遇到这些返回 NULL——**回收器、rmap、GUP 对这类映射全体跳过**。这就是 DPDK 大页被 pin 住、"内核不可见"的机制底层（12.5/ch13 的 VFIO/大页路线）。

## 6. 转换 API 速查（合法性判据）

| API | 合法域 | 禁区 |
|-----|--------|------|
| `__pa(va)` / `__va(pa)` | 直映区 | vmalloc/模块/用户 VA |
| `virt_to_page(va)` | 直映区（lowmem 语义） | 同上；HIGHMEM 页用 `kmap` 后仍非法 |
| `virt_to_phys` | 同 `__pa` | 同上 |
| `page_address(page)` | 直映页；HIGHMEM 例外需 kmap | vmalloc 页返回 NULL（用 `vmalloc_to_page`） |
| `vmalloc_to_page(va)` | vmalloc 区（walk 页表） | 直映区 |
| `follow_page()/GUP` | 任意用户映射 | 内核 VA |

## 7. HFT / 嵌入式关联

| 现象 | 机制兑现 |
|------|----------|
| DPDK/AF_XDP 大页模式 | 页被 pin + 用户直映射，struct page 仅作簿记，回收路径跳过 |
| `page_address()` 返回 NULL 的 crash | vmalloc 内存（如某些驱动 buffer）误当直映页——开 `DEBUG_VIRTUAL` 抓 |
| 树莓派外设寄存器访问 | 必须 `ioremap`，无 struct page、无 cache（映射属性在 PTE 设备位） |
| 内存开销核算 | struct page ≈ 物理内存 ×1.5%（64B/4KiB）——容量规划别忘 |
| cacheline 意识 | struct page 与数据页分离 = 顺序 DMA 预取也救不了元数据 miss |

## 8. 衔接

- [Ch 2 物理内存描述](../../chapter-02-describing-physical-memory/)：struct page 与 zone/node 的组织
- [§4 内核页表初始化](./section-4-内核页表初始化.md)：vmemmap 怎么在 boot 期建出来
- [06.5/ch06 folio](../../../06.5-modern-mm/chapter-06-page-cache-folio/)：v6.6 把 struct page 分组为 folio 的演进

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`virt_to_page()` 对 `kmalloc(64)` 返回的指针合法吗？**
A：合法。kmalloc 走 slab，对象物理页在直映区，对象 VA 也在直映区——`virt_to_page` 减 PAGE_OFFSET 得 PA 得 PFN 得 page。对 `vmalloc(64)` 的指针就是 **非法**：vmalloc VA 不在直映窗口（须用 `vmalloc_to_page`）。

**Q2：vmemmap 模式下，PFN 有洞时 `pfn_to_page` 会怎样？**
A：返回一个"地址合法但从未背靠物理页"的 struct page 指针——vmemmap 的该 section 表页没分配（vmemmap pop free 时跳过空洞）。解引用 = fault。所以必须 `pfn_valid()` 先行；非 vmemmap 的 classic sparse 则是 NULL-ish section 指针。**结论：PFN 不是许可证，pfn_valid 才是。**

**Q3：为什么 struct page 不放进 PTE，而是独立数组？**
A：PTE 是 per-mapping 的（同一页多处映射就有多份 PTE），struct page 是 per-物理页 的唯一状态。若塞进 PTE，共享页的 dirty/refcount 就有多副本一致性问题。分离 = 单一事实源 + rmap 反向桥接。

**Q4：`/proc/kpageflags` 一个字节都没有 struct page 之外的信息吗？**
A：它就是 vmemmap 元数据的用户态镜像（每 PFN 8B flags + 8B count + 8B mapping...分文件）。numastat/pagetypeinfo 同源。HFT 内存审计脚本可拿它统计 hugepage 实际分布。

**Q5：ARM64 直映区和 x86_64 的 PAGE_OFFSET 数值不同，跨架构代码怎么办？**
A：永远用宏（`__pa/__va/page_address`）而不是裸数值偏移；用户态程序根本看不到这两个区（内核 VA 空间）。跨架构 bug 高发点在驱动：对 ioremap 指针做 `virt_to_page`（在两架构都是错的，但 ARM64 上常常"碰巧"不崩，埋雷到 x86_64 才炸）。

</details>

---
