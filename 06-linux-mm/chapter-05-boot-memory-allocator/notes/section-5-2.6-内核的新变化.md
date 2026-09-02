# Ch 5 §5 2.6 内核的新变化（及 bootmem 的终结）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（bootmem 已删除，memblock 统一）

---

## 本节讲什么

本节回答一个问题：**原书说 2.6 对 bootmem「无架构级重写，只是小优化」，那从 2.6 到 v6.6，启动内存分配器到底发生了什么大事？**

答案是：**bootmem 从「小优化」走向了「被 memblock 彻底替换」**。这才是本节真正值得带走的「新变化」。

---

## 1. 原书的「2.6 小优化」（历史背景）

2.4 → 2.6，bootmem 没有架构级重写，主要是一个字段：

| 变化 | 作用 |
|------|------|
| `last_success` 字段（`bootmem_data_t`） | 记录上次成功分配位置，缩短后续扫描位图找空闲位的距离 |

与 `last_pos`/`last_offset` 同类：**减少 boot 路径上 O(n) 位图扫描**。但这类优化治标不治本——位图「逐页扫描」的根子还在。

---

## 2. 真正的新变化：bootmem → memblock（已删除 bootmem）

| 时间线 | 事件 |
|--------|------|
| 2.6.26 起 | **memblock** 引入（最初为 PowerPC 等架构服务） |
| 3.x | memblock 逐步接管 x86/arm 等主流架构，bootmem 被弃用 |
| **v4.20 前后** | **bootmem 彻底删除**（`mm/bootmem.c` 消失，Mike Rapoport 主导统一） |
| **v6.6** | 只剩 memblock，`alloc_bootmem*` 全族不复存在 |

**为什么 bootmem 非删不可**（呼应 §1）：

| 驱动因素 | 说明 |
|----------|------|
| **大内存** | 位图逐页扫描 O(n)，TB 级内存下启动变慢 |
| **NUMA 复杂化** | 位图表达不了「区间属性 + 节点归属」，memblock 的 `flags`/`nid` 天生支持 |
| **统一架构** | bootmem 在 x86/arm/PPC 各有实现分支，memblock 一套代码全架构通用 |

**结论：原书 §5 的「小优化」只是 bootmem 的临终优化，真正的故事是 memblock 的崛起与 bootmem 的消亡。**

---

## 3. v6.6 memblock 的现代特性（相对原书的新能力）

| 特性 | 说明 |
|------|------|
| **NUMA 感知分配** | `memblock_alloc_try_nid(..., nid)` 直接在指定节点分配（§3） |
| **区间 flags** | `HOTPLUG`/`MIRROR`/`NOMAP` 表达热插拔/镜像/不映射语义（§1） |
| **top-down / bottom-up** | `memblock.bottom_up` 控制分配方向（§3） |
| **`physmem` 第三表** | `CONFIG_HAVE_MEMBLOCK_PHYS_MAP` 下额外维护「物理内存映射」表，区分「内存」与「内存映射」 |
| **debug 导出** | `CONFIG_ARCH_KEEP_MEMBLOCK` 下 `/sys/kernel/debug/memblock` 导出两表 |
| **`memblock_free_late`** | 退役后仍可按页还 Buddy（§4） |

---

## 4. bootmem/memblock → Buddy 一图（全章收束）

```
        上电 / arch setup（firmware 报告 e820/DT/EFI）
              │
              ▼
    memblock_add*  ──► memory 表（可用区间 + nid + flags）
              │
              ▼
    memblock_reserve* ──► reserved 表（内核代码/initrd/crashkernel/页表…）
              │
              ▼
    memblock_alloc* ──► 页表、struct page(memmap)、临时结构 …（清零 + 对齐）
              │
              ▼
    mem_init() → memblock_free_all()  ← ★ 退役分水岭
              │
              ▼
    空闲页框 ──► Buddy（Ch6）──► 此后所有 runtime 分配
```

---

## 5. HFT / 阅读建议

| 读者 | 建议 |
|------|------|
| **HFT 工程** | **可跳过正文**；知道「运行时内存来自 Buddy + slab，memblock 仅启动几百毫秒」即可 |
| **读内核启动 / 调试** | 理解 `memblock_reserve` 与 `/proc/iomem` 中 `Kernel code`/`reserved` 从哪来，用 `memblock=debug` 追 |
| **继续精读** | [Ch6 物理页分配](../../chapter-06-physical-page-allocation/) · Ch1 路线的 [`page_alloc.c`](https://elixir.bootlin.com/linux/latest/source/mm/page_alloc.c) |

---

## 6. 衔接

- 下一章：[Ch6 物理页分配](../../chapter-06-physical-page-allocation/)（Buddy 的正篇）
- 回收闭环：[Ch10 页框回收](../../chapter-10-page-frame-reclamation/)（Buddy 分配不出时反向回收）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：原书说 bootmem「无架构级重写」，但为什么它最终还是被删了？**
A：因为「无重写」恰恰是问题——bootmem 的**位图逐页扫描**模型在大内存 + NUMA 下根本跟不上，小优化（`last_success`）治标不治本。memblock 的「区间表」模型从根子上解决了扩展性，于是 bootmem 被逐步替换，最终在 v4.20 前后删除。**「没重写」不是稳定，是积重难返。**

**Q2：memblock 相比 bootmem，对「NUMA 复杂化」的适配好在哪里？**
A：bootmem 位图只有「free/used」二元态，表达不了「这段内存属于哪个节点」；memblock 的 `memblock_region` 带 `nid` 字段（`CONFIG_NUMA` 下），一个区间直接携带节点归属。分配时 `memblock_alloc_try_nid(..., nid)` 就能在指定节点分配，这正是 NUMA 初始化（Ch2）需要的能力。

**Q3：`CONFIG_HAVE_MEMBLOCK_PHYS_MAP` 下的「physmem 第三表」和 `memory` 表有什么区别？**
A：`memory` 表记录「**可用物理内存**」（能分配给内核用）；`physmem` 表记录「**物理内存映射**」（更广的、包括被设备/固件占用的物理区间）。某些架构需要区分「内存本身」和「内存的映射关系」，于是额外维护这张表。默认 x86 通常不开这个选项。

**Q4：`memblock_discard()` 释放 memblock 后，为什么它的 API 都带 `__init` 标记？**
A：因为 memblock 的代码和数据只在启动期有效。`__init` 把函数放进一个特殊的段，**退役后这段内存被整体回收**（`free_initmem()`）。这样既能保证「退役后没人误用 memblock」（段都没了），又能把启动专用的几十 KB 内存还给系统。

**Q5：为什么说「bootmem → memblock」是理解 Ch5 的「唯一主线」？**
A：因为原书五节全在讲 bootmem，而 v6.6 里 bootmem 已经不存在了。如果照本宣科背 bootmem 的 `node_bootmem_map`/`last_success`，读 v6.6 源码会完全对不上。正确读法：**把原书的 bootmem 概念当成「历史对照」，每个概念都映射到 memblock 的对应物**——bitmap→region list、init_bootmem_core→memblock_add/reserve、alloc_bootmem→memblock_alloc、mem_init 退役→memblock_free_all。

</details>

---
