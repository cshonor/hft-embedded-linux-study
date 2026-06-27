# Ch 14 结束语 · The Final Word

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过**（全书 **理论部分收尾** — 读 **§4 全局交互图** 复盘 Ch 2–13；之后进入 **附录 A–M Code Commentary**）

---

## 1. 复杂性与理论落地的困难

内存管理 **庞大、复杂、耗时** — 且 **很难把理论直接搬进真实内核**：

| 难点 | 说明 |
|------|------|
| **多道程序环境** | 调度、分页、**多进程交互** 同时发生 |
| **建模困难** | 难以 **精确预测** 真实系统行为（workload 一变，结论就变） |

**HFT 启示：** 论文里的 **最优替换策略** 在你的 **订单簿 + 绑核 + mlock** workload 上 **未必最优** — 必须 **测量**（延迟分位数、vmstat、perf），不能 **只信公式**。

---

## 2. 实践：直觉、模拟与调优

理论不够用时，内核开发者靠：

| 手段 | 作用 |
|------|------|
| **直觉 (intuition)** | 指导 **先做什么结构** |
| **Workload 模拟** | 在 **特定负载** 下验证算法（如 page replacement） |
| **管理员调优** | **`sysctl`、swap、overcommit、cgroup** — 适应 **不同部署** |

**页面替换** 是研究最多的领域之一，却 **只在某些 workload 下可证「好」** — **算法 + 运维** 缺一不可。

→ 对应本书 **Ch 10 LRU/active-inactive** — 工程折中，非教科书 LRU。

---

## 3. 本书的核心目的

Mel Gorman 写本书是为了 **弥合鸿沟**：

```
内存管理理论  ←—— 本书 ——→  Linux VM 真实代码
                    │
            尽量架构无关地讲清机制
```

| 读者收获 | 内容 |
|----------|------|
| **理论部分（Ch 1–14）** | Node/Zone/Page、页表、VMA、Buddy、Slab、回收、Swap、shmem、OOM |
| **Code Commentary（附录 A–M）** | 与正文对应的 **源码走读** |

**本仓库笔记：** 各章 `chapter-*.md` 已按 **HFT 落地**（NUMA、mlock、对象池、延迟）加注；附录正文多 **待走读**。

---

## 4. VM 子系统交互全局概览（对应原书图 14.1）

宏观上，**一次用户态访问内存** 可能穿过 **整条链**；**一次 `malloc`/fault** 则从下往上 **要页**：

```
┌─────────────────────────────────────────────────────────────────┐
│  用户进程：mmap / brk / touch VA / MAP_SHARED IPC               │
└────────────────────────────┬────────────────────────────────────┘
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│  Ch 4  mm_struct · VMA · page fault · COW · copy_*_user         │
└────────────────────────────┬────────────────────────────────────┘
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│  Ch 3  PGD/PMD/PTE · TLB flush · rmap                           │
└────────────────────────────┬────────────────────────────────────┘
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│  Ch 2  struct page · Zone · Node · LRU flags · pageset           │
└────────────────────────────┬────────────────────────────────────┘
                             ▼
        ┌────────────────────┼────────────────────┐
        ▼                    ▼                    ▼
   Ch 6 Buddy          Ch 8 Slab            Ch 7 vmalloc
   alloc_pages         kmalloc              虚拟连续/物理散
        ▲                    ▲
        │                    │
   Ch 5 bootmem/memblock 退役移交
        │
   内存不足时 ↑
        │
   Ch 10 shrink · kswapd · page cache
        ├─► Ch 11 swap（匿名页）
        └─► Ch 12 shmem/tmpfs（共享「文件」页）
        │
   仍不足 ► Ch 13 OOM killer
```

### 按章快速索引（理论部分）

| 章 | 主题 | 笔记 |
|:--:|------|------|
| 1 | 读源码 / 补丁 / `mm/` 路线 | [chapter-01](./chapter-01-简介.md) |
| 2 | Node · Zone · `struct page` | [chapter-02](./chapter-02-描述物理内存.md) |
| 3 | 页表 · TLB · rmap | [chapter-03](./chapter-03-页表管理.md) |
| 4 | VMA · fault · mlock | [chapter-04](./chapter-04-进程地址空间.md) |
| 5 | bootmem → Buddy | [chapter-05](./chapter-05-启动内存分配器.md) |
| 6 | 伙伴系统 · GFP | [chapter-06](./chapter-06-物理页分配.md) |
| 7 | vmalloc | [chapter-07](./chapter-07-非连续内存分配.md) |
| 8 | Slab · kmalloc · mempool | [chapter-08](./chapter-08-Slab分配器.md) |
| 9 | HIGHMEM · kmap · bounce | [chapter-09](./chapter-09-高端内存管理.md) |
| 10 | LRU · kswapd · reclaim | [chapter-10](./chapter-10-页框回收.md) |
| 11 | swap · swp_entry | [chapter-11](./chapter-11-交换管理.md) |
| 12 | tmpfs/shmem · SysV shm | [chapter-12](./chapter-12-共享内存虚拟文件系统.md) |
| 13 | OOM | [chapter-13](./chapter-13-内存耗尽管理.md) |

### HFT 精读捷径（相对作者 Ch1 源码路线）

| 路径 | 章节 |
|------|------|
| **延迟 / 布局** | Ch 2 → 3 (+ [THP](./note-透明大页THP.md)) → 8 → 4 → 10 |
| **读 `mm/` 源码** | Ch 1：`oom_kill.c` → `vmalloc.c` → `page_alloc.c` → `mmap.c` |

---

## 5. 作者期许与后续

- 读完 **理论（Ch 1–14）** 再进 **附录 Code Commentary** — 应对 **VM 子系统更有信心**。
- 鼓励社区为 **其他内核子系统** 写同类 **理论 + 代码** 著作。

**你这边下一步（可选）：**

1. **附录 A–M** — 按 Elixir 走读 `mm/*.c`，与正文章节对照。  
2. **HFT 落地** — [01-CSAPP Ch9](../01-CSAPP-3rd/chapter-09-virtual-memory/) · [08-TLPI](../08-The-Linux-Programming-Interface/) · [10-DPDK 大页](../14-DPDK-Low-Latency-Network/) · `numactl` / `mlock` / `perf`。  
3. **交叉** — [05-LKD Ch12/15](../05-Linux-Kernel-Development/00_Book_3rd_Notes/) · [06-ULK Ch8–9](../06-Understanding-Linux-Kernel/)

---

## 相关章节

- 上一章：[chapter-13-内存耗尽管理.md](./chapter-13-内存耗尽管理.md)
- 附录入口：[appendix-A-简介.md](./appendix-A-简介.md)
- 全书目录：[OUTLINE.md](./OUTLINE.md) · [README.md](./README.md)

---
