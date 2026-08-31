# Ch 3 §4 内核页表初始化 (Kernel Page Tables)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`mm/memory.c` 的 `init_mm`、`arch/x86|arm64` 的启动序列）

---

## 本节讲什么

分页不是上电就有：从"CPU 以物理地址跑固件"到"内核运行在稳定页表上"是一条分阶段的引导链。本节讲清 **每阶段谁建表、建多大、用什么分配器**——并把原书 2.4 叙事对齐到 v6.6 + ARM64（树莓派/服务器两条线都覆盖）。

---

## 1. 为什么要分阶段

鸡生蛋问题：**页表分配器（buddy/slab）本身要跑在分页上**。解法 = 自举（bootstrapping）：先用手搓的最小页表开启 MMU，再用简单分配器建全量映射，最后退役它。

| 阶段 | 原书（2.4 · x86） | v6.6 现实 |
|------|--------------------|-----------|
| ① 固件→内核 | 真实模式→保护模式，`swapper_pg_dir` 映射前 8MiB | x86_64：`__startup_64` 修复 2MiB 大页初始映射；ARM64：head.S 以**块映射**（2MiB/1GiB section）直映内核镜像+DTB+initrd |
| ② 开 MMU | `cr3 = swapper_pg_dir` | x86 写 CR3；ARM64 写 TTBR0_EL1（v6.6 起内核在高 VA 走 **TTBR1**，KPTI 后彻底分离） |
| ③ 建全量映射 | `paging_init()`→`pagetable_init()` | x86 `init_mem_mapping`；ARM64 `paging_init()`→`map_mem()` 扫 memblock 逐段块映射 |
| ④ 退役自举分配器 | bootmem → buddy | **memblock** → buddy（见 06.5/ch01） |

## 2. `init_mm`：内核的"进程描述符"

内核自身页表的拥有者不是任何进程，是 **0 号上下文 `init_mm`**：

```c
/* mm/init-mm.c（v6.6） */
struct mm_struct init_mm = {
    .pgd    = swapper_pg_dir,          /* x86 名字；ARM64 是 init_pg_dir */
    .mm_mt  = MTREE_INIT_EXT(mm_mt, ...) /* v6.6：内核 VMA 也在 maple tree 上 */
};
```

| `init_mm` 事实 | 意义 |
|----------------|------|
| 无 `mm_users`（不引用计数） | 永生，与内核同寿 |
| 内核线程 `borrow` 它 | `active_mm = init_mm` → **lazy TLB**：内核线程不切 CR3/TTBR，省一次 full flush |
| `pte_alloc_one_kernel(&init_mm)`（memory.c:449） | 内核态建表（如 fixmap/vmalloc）用它 |

## 3. ARM64 启动建表（树莓派 5 这条线）

```
ROM → SPL/uboot（物理地址裸跑）
  → head.S：建 init_pg_dir，块映射内核镜像（.text/.rodata/.data 分段设权限）
  → 开 MMU（TTBR0_EL1 = init_pg_dir）
  → start_kernel → setup_arch → paging_init()
       → map_mem()：扫 memblock.memory 每个区间
       → 用 2MiB/1GiB 块映射直映全部 DDR（linear map）
  → mm_core_init → memblock 交棒给 buddy
```

**linear map 权限细粒度**：v6.6 ARM64 支持 `rodata=full` 时把直映拆到 4KiB 粒度设 NX/RO（防 ret2dir 攻击）——代价是 **页表页数量暴涨**，`vmemmap` 与 linear map 的表页本身吃掉几百 MiB（大内存机器）。默认 `rodata=on` 折中（2MiB 粒度）。

**HFT 视角**：开机后 `[dmesg] Memory: ... available` 与物理条数之差中，一块就是页表自身开销。

## 4. v6.6 与原书的差异清单

| 原书（2.4/x86-32） | v6.6 |
|---------------------|------|
| 内核与用户共享一个 PGD 的高端 | KPTI（x86）/ ARM64 TTBR0/TTBR1 分离：**内核映射不在用户页表里**（meltdown 缓解） |
| 8MiB 引导映射够用 | 内核镜像+initrd+DTB+earlycon 都要在开 MMU 前块映射 |
| bootmem 位图分配器 | **memblock**（type: memory/reserved 两条链）→ 06.5/ch01 |
| `swapper_pg_dir` 一张表管内核 | vmalloc/fixmap/kasan/vmemmap 各有独立子树，全挂在 `init_mm->pgd` 下 |

## 5. 启动后还能改内核页表吗——能，vmalloc 就是

内核页表 **不是只读档案**：

- `vmalloc()`/`ioremap()` 动态往 vmalloc 区插 PTE——**这就是要锁** 的场景（`vmap_area_lock` + 页表锁）
- BPF/JIT、模块加载、`kasan` 影子内存都走这条路
- 修改 `init_mm` 的表项 **不会自动出现在用户进程页表里**：vmalloc 区是 **共享子树**（所有进程 PGD 的内核部分指到同一批下层表）——fork 时只复制用户部分（`clone_pg_range` 跳过内核 VA）

## 6. HFT / 嵌入式关联

| 现象 | 机制兑现 |
|------|----------|
| 内核线程不污染 TLB | `active_mm=init_mm` + lazy TLB：上下文切换省 CR3/TTBR0 写 |
| KPTI 开销 | 每次用户↔内核切换换页表基址（双 CR3/TTBR）→ 单边 ~100ns 级；**HFT 裸金属部署常 `pti=off`**（自担 meltdown 风险） |
| 树莓派调内存布局 | `cat /proc/iomem` 看 linear map/vmalloc 划分；vmalloc 区 OOM → `vmalloc=xxx` 参数 |
| hugepage 预留发生在哪 | `hugepages=N` 在 boot memblock 阶段就 **先扣走**（`hugetlb` early reserve）——所以 2MiB 大页池与运行期 CMA/THP 互不干扰 |

## 7. 衔接

- [§5 地址与 struct page 的映射](./section-5-地址与-struct-page-的映射.md)：vmemmap 在启动期如何建
- [Ch 5 启动内存分配器](../../chapter-05-boot-memory-allocator/)：memblock 前身 bootmem 的原书机制
- 现代版：[06.5/ch01 memblock](../../../06.5-modern-mm/chapter-01-physical-memory-memblock/)
- ARM64 实战：[07-arm-architecture](../../../07-arm-architecture/)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么内核线程不分配自己的 `mm_struct`？**
A：内核线程永远跑在内核 VA（`init_mm` 共享子树），用户地址空间对它无意义。省掉的是：页表页（每 mm 一份 PGD 起）、TLB 切换（借 `active_mm` 不换表基址）。切换回用户进程时若 mm 相同则连 flush 都免——这是 05-linux-kernel 调度章节 lazy TLB 的伏笔。

**Q2：KPTI 下用户进程页表里还有内核映射吗？**
A：只剩最小 trampoline（入口跳板+SYSCALL 存根），完整内核映射在独立 kernel table（x86 PCID 分离 / ARM64 走 TTBR1 本来就分离）。副作用：syscall 进出多一次页表切换。ARM64 从一开始就是 TTBR0/TTBR1 双基址架构，KPTI 对它近乎免费（v8.0 PAN 时代即可），x86 才是真付代价的。

**Q3：vmalloc 的 PTE 建在哪个 mm 里？别的进程怎么看见？**
A：建在 `init_mm` 的表里。所有进程 PGD 的内核半区 **指向同一物理下层表**——fork 只复制用户半区。所以 vmalloc 一处建立、全体可见，无需广播。（用户态映射则完全私有。）

**Q4：`hugepages=1024` 预留的大页，运行期物理内存紧张能被回收吗？**
A：不能自动回收成 buddy 页（除非 `echo > nr_hugepages` 主动缩池）。boot 期从 memblock 预留或启动早期从 buddy 抓取（`gather_bootmem_prealloc`），池子独立。HFT 常用 boot 参数预留而非运行期 echo——后者碎片化后可能失败。

**Q5：`init_mm` 的 VMA 什么时候用？**
A：内核自身映射管理：`vread()`（/dev/kmem 读 vmalloc 区）、`register_kernel_stack`（v6.6 尚无）、vmalloc 查区间。v6.6 起内核 VMA 也在 maple tree（`mm_mt`）上，与用户 VMA 同一数据结构——代码统一化的一步。

</details>

---
