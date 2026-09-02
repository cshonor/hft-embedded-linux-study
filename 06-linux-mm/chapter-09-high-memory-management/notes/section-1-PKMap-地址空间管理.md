# Ch 9 §1 PKMap 地址空间管理

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`arch/x86/include/asm/highmem.h` 的地址布局注释）

---

## 本节讲什么

本节回答一个问题：**为什么 32 位内核要在页表顶部划一块「PKMap 窗口」，它的地址布局长什么样？**

关键前提先说清：**PKMap 是 `CONFIG_HIGHMEM`（32 位）专属概念**。x86_64 没有 HIGHMEM，也没有 PKMap 窗口——整个窗口「不存在」。本节以 x86 32 位的真实布局为基准讲解，并明确它在 64 位上的归宿。

---

## 1. PKMap 窗口：临时映射稀缺槽位

内核在**内核地址空间顶部**保留一块 **Persistent Kernel Mapping (PKMap)** 窗口，用来**临时**把 HIGHMEM 物理页映射进内核可访问的虚拟地址：

```
（arch/x86/include/asm/highmem.h 里的权威注释，high memory on 分支）
    高地址
    FIXADDR_TOP          ─── 固定映射区顶部
        fixed addresses        （APIC、early_ioremap 等）
    FIXADDR_START        ─── 固定映射区
        temp fixed / persistent kmap area
    PKMAP_BASE           ═══ PKMap 区（持久内核映射窗口）
        vmalloc area
    VMALLOC_START        ─── vmalloc 区
    high_memory          ─── 常规内核线性映射顶部（= PAGE_OFFSET 之上的直接映射）
```

| 概念 | 说明 |
|------|------|
| `PKMAP_BASE` ~ `FIXADDR_START` | **临时**把 HIGHMEM 物理页映射进内核可访问 VA 的窗口 |
| 池很小 | 同时约 **1024 个**高端页映射（`LAST_PKMAP`），**必须短借短还** |
| `PKMAP_NR(virt)` / `PKMAP_ADDR(nr)` | 虚拟地址 ↔ 槽位号互转（`highmem.h`） |

```c
/* arch/x86/include/asm/highmem.h */
#define LAST_PKMAP_MASK (LAST_PKMAP-1)
#define PKMAP_NR(virt)  ((virt-PKMAP_BASE) >> PAGE_SHIFT)   /* VA → 槽位号 */
#define PKMAP_ADDR(nr)  (PKMAP_BASE + ((nr) << PAGE_SHIFT)) /* 槽位号 → VA */
```

**设计约束**：PKMap 是**稀缺槽位**——占着不 `kunmap` 会**饿死**其他需要 kmap 的路径（尤其是文件系统、网络栈这些高频临时映射）。所以 PKMap 的纪律是「**短借短还，绝不跨睡眠**」。

---

## 2. 为什么需要 PKMap：HIGHMEM 的困境

回顾 Ch2 §4：32 位内核 3:1 分割，内核只有 **1GB 虚拟地址空间**，其中**直接映射区只能覆盖 ~896MB 物理内存**。当物理内存 > 896MB，多出来的部分是 **HIGHMEM**——**内核无法通过 `PAGE_OFFSET + 物理地址` 直接访问**。

```
32 位内核虚拟地址空间（1GB）
  ├─ 直接映射区（~896MB）── 覆盖 LOWMEM，__va(pfn) 直接可达
  └─ 顶部 ~128MB ────────── 划给 vmalloc / PKMap / fixmap
                                │
                                └─ HIGHMEM 物理页要访问，只能"临时映射"进 PKMap
```

**所以 PKMap 的本质**：给「内核够不着的 HIGHMEM 页」开一扇**临时的、共享的、稀缺的**窗。

---

## 3. x86_64 上：PKMap 窗口「消失」

在 64 位内核上，`CONFIG_HIGHMEM` **未定义**（Ch2 §4 已实证），整个 HIGHMEM 体系（含 PKMap）都不编译。`arch/x86/include/asm/highmem.h` 的注释给出对比：

```
high memory on:                        high memory off (x86_64):
  FIXADDR_TOP                            FIXADDR_TOP
    fixed addresses                        fixed addresses
  FIXADDR_START                          FIXADDR_START
    temp fixed/persistent kmap area        VMALLOC_END
  PKMAP_BASE  ← 这一块没了               temp fixed/vmalloc area
    vmalloc area                        VMALLOC_START
  VMALLOC_START                          high_memory
  high_memory
```

**结论**：64 位直接映射区覆盖**全部**物理内存（`PAGE_OFFSET` 之上 128TB 窗口，远大于物理 RAM），**没有「够不着」的页**，自然不需要 PKMap。PKMap 是「32 位地址空间太小」这个历史包袱的解药——**包袱没了，解药也就退休了**。

---

## 4. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| x86_64 HFT 网关机 | 无 HIGHMEM、无 PKMap，`kmap` 是 no-op（§2）——不必担心「占着 kmap 槽」 |
| 32 位嵌入式 / 旧设备 | PKMap 稀缺槽位是真实约束，`kmap` 跨睡眠会饿死其他路径 |
| 理解「临时内核 VA 窗口」思想 | 即使 64 位无 PKMap，这个「稀缺窗口 + 短借短还」思想仍指导 fixmap/临时映射 |

---

## 5. 衔接

- 下节 [§2 映射与解除映射高端页](./section-2-映射与解除映射高端页.md)：`kmap` 家族 API 及 64 位上的 no-op
- HIGHMEM 来由：[Ch2 §4 高端内存](../../chapter-02-describing-physical-memory/notes/section-4-高端内存.md)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 PKMap 是「稀缺槽位」，而不是给每个 HIGHMEM 页永久映射？**
A：因为 32 位内核地址空间总共才 1GB，顶部 128MB 要同时容纳 vmalloc、fixmap、PKMap 等。给每个 HIGHMEM 页永久映射意味着「有多少 HIGHMEM 页就要多少内核 VA」——那 1GB 根本不够分。所以 PKMap 只能是「~1024 个槽位的共享窗口」，用完即还。

**Q2：`PKMAP_NR` 和 `PKMAP_ADDR` 这对宏是干嘛的？**
A：它们做**虚拟地址 ↔ 槽位号**的双向换算。`PKMAP_NR(virt)` 把 PKMap 区里的一个 VA 换算成「第几个槽位」（`(virt - PKMAP_BASE) >> PAGE_SHIFT`）；`PKMAP_ADDR(nr)` 反过来。内核用槽位号做数组索引（`page_address_maps[]`），用 VA 做实际访问。

**Q3：x86_64 上 PKMap 窗口「消失」的直接证据是什么？**
A：`CONFIG_HIGHMEM` 未定义 → `arch/x86/include/asm/highmem.h` 里「high memory off」分支根本没有 `PKMAP_BASE` 这一层（布局从 FIXADDR_START 直接到 vmalloc 区）。整个 HIGHMEM 体系（含 PKMap、`kmap_high`、`LAST_PKMAP`）都不编译。

**Q4：为什么「直接映射区覆盖全部物理内存」就能废除 PKMap？**
A：PKMap 存在的唯一理由是「有些物理页（HIGHMEM）不在直接映射区，内核够不着」。64 位直接映射区有 128TB 虚拟窗口，远超物理 RAM 总量，**每一页物理内存都有固定的 `PAGE_OFFSET + pfn` 虚拟地址**，随时可访问，自然不需要临时映射窗口。

**Q5：PKMap 和 fixmap 都是「顶部窗口」，区别在哪？**
A：PKMap 是**动态、共享、可排队等待**的（kmap 满时睡眠等槽）；fixmap 是**静态、编译期固定、每 CPU**的（如 APIC 映射地址编译期就定死）。PKMap 管「HIGHMEM 页的临时映射」，fixmap 管「少数几个固定硬件地址的永久映射」。两者都在内核地址空间顶部，但语义完全不同。

</details>

---
