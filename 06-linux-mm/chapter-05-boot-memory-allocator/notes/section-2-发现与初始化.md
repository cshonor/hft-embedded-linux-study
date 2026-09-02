# Ch 5 §2 发现与初始化 (Initializing)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/memblock.c` 的 `memblock_add` / `memblock_reserve`）

---

## 本节讲什么

本节回答一个问题：**内核怎么知道「这台机器有多少内存、哪些能用」？**

原书的答案是「arch 探测 `min_low_pfn`/`max_low_pfn` → `init_bootmem_core()` 填位图」；v6.6 的答案是「**firmware 报告内存表 → `memblock_add()` 填 `memory` 表 → 各子系统 `memblock_reserve()` 填 `reserved` 表**」。两表怎么被填满，就是本节内容。

---

## 1. 架构相关探测：内存信息从哪来

内核不会自己「摸」内存，它**问 firmware**。不同架构问的对象不同：

| 架构 | firmware 接口 | 内存信息形态 |
|------|--------------|-------------|
| **x86_64** | BIOS/UEFI 的 **e820 表** | 一段段 `[base, size, type]`（type 区分 usable/reserved/ACPI…） |
| **arm64** | **设备树 (DT/ACPI)** 的 `memory` 节点 | `reg = <base size>` 列表 |
| 通用 | EFI **memory map** | UEFI 统一的内存描述符 |

x86_64 上，早期 `setup_arch()` 把 e820 表转成 `min_low_pfn`/`max_low_pfn`（原书）或直接喂给 memblock（v6.6）。**「探测物理内存边界」这一步没有变，变的是「探测结果存哪」——从逐 PFN 的边界变量，变成一段段 memblock 区间。**

---

## 2. v6.6：`memblock_add()` 填 `memory` 表

拿到 firmware 报告后，内核把每段可用内存**注册进 `memblock.memory`**：

```c
/* mm/memblock.c:727 */
int __init_memblock memblock_add(phys_addr_t base, phys_addr_t size)
{
    phys_addr_t end = base + size - 1;
    return memblock_add_range(&memblock.memory, base, size,
                              MAX_NUMNODES, 0);
}
/* NUMA 版：带节点号 */
int __init_memblock memblock_add_node(phys_addr_t base, phys_addr_t size,
                                      int nid, enum memblock_flags flags); /* :705 */
```

**`memblock_add_range()` 的职责**（`memblock.c:587`）：把新区间**插入有序数组**，并处理三种重叠情况——**合并相邻区间、部分重叠则拆分、完全包含则忽略**。最终 `memory.regions[]` 保持「按 base 排序、互不重叠」的不变量。

```
firmware:   [0x1000, 0x9F000) usable   [0x100000, 0x80000000) usable
                     │                         │
                     ▼                         ▼
memory 表:   region[0] = {0x1000, 0x9E000}  region[1] = {0x100000, ...}
              （排序、合并、去重叠后）
```

---

## 3. `memblock_reserve()` 填 `reserved` 表

可用内存注册完，各子系统**立刻开始圈地**，把「这段我要用」写进 `memblock.reserved`：

```c
/* mm/memblock.c:871 */
int __init_memblock memblock_reserve(phys_addr_t base, phys_addr_t size)
{
    return memblock_add_range(&memblock.reserved, base, size,
                              MAX_NUMNODES, 0);
}
```

启动早期被 reserve 的典型对象：

| 保留对象 | 谁在 reserve | 为什么 |
|----------|-------------|--------|
| **内核镜像**（text/data/bss） | 早期 setup | 内核自己占的物理页不能被分配 |
| **initrd/initramfs** | `setup_arch` | 解压后的临时根文件系统 |
| **crashkernel** | `setup_arch` | kdump 备用内存（内核崩溃时用来抓现场） |
| **页表 / `struct page`(memmap)** | `paging_init` 等 | 这些**元数据本身**也要占物理页 |
| **firmware/ACPI 表** | 早期 | 不能覆盖 firmware 的数据 |

**关键直觉（与原书同构，但更清晰）**：原书 `init_bootmem_core()` 是「**先全保留，再逐步 free**」；memblock 是「**先全加进 memory，再逐项 reserve**」——两条路**逻辑等价**，最终都得到「可用 = memory − reserved」这个差集。memblock 的「两表相减」比「单表翻转」更直观，也更好调试。

---

## 4. 观察内存布局：`/proc/iomem` 与 `memblock=debug`

这两表最终会体现在 **`/proc/iomem`**（运行时）和早期日志里：

```
$ cat /proc/iomem | head
00000000-00000fff : Reserved          ← firmware 保留
00100000-9d3eafff : System RAM        ← 可用（曾进 memblock.memory）
  01000000-023fffff : Kernel code     ← 曾 reserve
  02400000-0269ffff : Kernel rodata
  02700000-02ffffff : Kernel data
100000000-4a3ffffff : System RAM      ← 高段内存
  3f8000000-3f8ffffff : Crash kernel  ← crashkernel reserve 的痕迹
```

**调试技巧**：启动参数加 `memblock=debug`，早期内核会打印每次 `add`/`reserve` 的区间和调用者（`_RET_IP_`），是追「这段 reserved 是谁干的」的第一手证据。

---

## 5. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 排查「为什么可用内存比物理内存少」 | `memblock_reserve` 圈走了 crashkernel/initrd/memmap，看 `/proc/iomem` 或 `memblock=debug` |
| 树莓派/嵌入式裁剪 crashkernel | 少 reserve 一段 crashkernel 就多一段可用内存，boot 时决定 |
| 理解 NUMA 节点边界从哪来 | `memblock_add_node(..., nid)` 是 `node_start_pfn` 的最初来源（§1 Q5） |

---

## 6. 衔接

- 下节 [§3 内存分配与释放](./section-3-内存分配与释放.md)：两表就绪后，怎么分配
- 物理页分配：[Ch6 物理页分配](../../chapter-06-physical-page-allocation/)（memblock 退役后的接管者）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`memblock_add()` 和 `memblock_reserve()` 的实现几乎一模一样，区别只在操作哪张表，对吗？**
A：对。两者都调 `memblock_add_range()`，区别是**第一参数**——`add` 传 `&memblock.memory`，`reserve` 传 `&memblock.reserved`。语义上 `add`=「声明这里有可用内存」，`reserve`=「声明这段被占用」。这正是「两表模型」的优雅之处：**同一套区间管理逻辑，两张表复用**。

**Q2：`memblock_add_range()` 为什么要处理「重叠」？**
A：firmware 报的内存段可能**重叠或相邻**（e820 常有碎段），且不同子系统可能对同一段重复 reserve。`add_range` 维护「有序、互不重叠」的不变量——相邻则合并、部分重叠则拆成几段、完全包含则忽略——保证后续「找空闲」逻辑只需在干净数组上遍历。

**Q3：原书的「先全保留再 free」和 memblock 的「先全加再 reserve」为什么等价？**
A：两条路最终都表达「可用 = 全部内存 − 被占用部分」。bootmem 从「全 1（全占用）」出发，把已知空闲的 bit 清零；memblock 从「全进 memory 表」出发，把占用段加进 reserved 表。结果一致，只是**翻转的时机和数据结构不同**——memblock 的差集模型更易理解和调试。

**Q4：怎么快速定位「这段 reserved 是谁圈走的」？**
A：启动参数加 `memblock=debug`，内核会打印每次 `add`/`reserve` 的区间和**调用者地址**（`_RET_IP_`，`%pS` 符号化后能直接看到函数名）。运行时则可看 `/proc/iomem` 里各段的子区间标签（`Kernel code`/`Crash kernel`…）反推。

**Q5：crashkernel 为什么要在启动早期 reserve，而不是运行时再分配？**
A：crashkernel 是给 **kdump** 用的——内核崩溃时，kexec 起的**第二个内核**需要一段**物理连续、且绝不被污染**的内存来抓现场。这段内存必须在**常规分配器接管之前**就圈死，否则早被 Buddy/slab 分掉，崩溃时就无「干净内存」可用。所以它是 `memblock_reserve` 的常客。

</details>

---
