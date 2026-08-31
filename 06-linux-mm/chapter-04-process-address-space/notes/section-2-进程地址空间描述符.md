# Ch 4 §2 进程地址空间描述符（`mm_struct` · v6.6 版）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`include/linux/mm_types.h` :18 `mm_mt`、:31 `task_size`、:32 `pgd`、:91 `mmap_lock`）

---

## 本节讲什么

`mm_struct` 是地址空间的户口本：页表根、VMA 树、统计、锁全在这。v6.6 相对原书有三个实质变化——**VMA 树换 maple tree**、**mmap_lock 的 cacheline 工程**、**引用计数语义细分**。本节逐字段过一遍，重点讲锁。

---

## 1. 骨架（v6.6 实锚）

```c
struct mm_struct {
    struct {                                /* 嵌套 anon struct：cache 布局工程 */
        struct maple_tree mm_mt;            /* :18  VMA 树（原书：链表+红黑树）*/
        ...
        unsigned long task_size;            /* :31 用户 VA 上限 */
        pgd_t *pgd;                         /* :32 页表根（Ch3） */
        ...
        struct rw_semaphore mmap_lock;      /* :91 VMA/页表并发锁 */
    };
    atomic_t mm_users;                      /* 用户数（线程数语义）*/
    atomic_t mm_count;                      /* 引用数（含借用者）*/
    ...
    unsigned long hiwater_rss, hiwater_vm;
    unsigned long total_vm, data_vm, ...;
};
```

## 2. 三大件逐个说

### ① VMA 树：maple tree（`mm_mt`）

| 原书（rbtree + 链表） | v6.6 maple tree |
|------------------------|------------------|
| 查找 O(log n)，cache 不友好（指针 chasing） | R-range B-tree，节点多槽（16 槽）→ 树更矮、**cache 命中更好** |
| 遍历用链表 | `mas_find()`/`mas_walk()`（mmap.c:1111/:1581 实锚） |
| 写锁粒度粗 | 支持写者并发优化（v6.6 持续演进） |

**读码换算：** 原书 `mm->mmap` 链表遍历 → v6.6 `mas_for_each`；`rb_find` → `mas_walk`。

### ② 页表根 `pgd` + `task_size`

- `pgd` 就是 Ch3 一切的出发点；fork 时复制（COW 语义在表项级）
- `task_size` 由 arch 定（x86_64 128TiB）；`mmap` 上界 = `task_size - GAP`

### ③ `mmap_lock`（原书叫 `mmap_sem`，v6.6 是 rw_semaphore）

**v6.6 在 mm_types.h 里专门写了 cacheline 注释**（:80-:88 实锚大意）：锁高争用时，锁字段与 pgd 等热字段的 cacheline 布局会影响性能，内核刻意把它们隔开——**连 struct 字段排序都在为 SMP 争用做工程**。

| 拿读锁 | 拿写锁 |
|--------|--------|
| 缺页 handler、GUP 慢路径、`/proc/pid/maps` 读 | mmap/munmap/mprotect、fork、khugepaged collapse |
| 多线程并发 OK | 排他 |

**HFT 要害——写锁的三大延迟事件源：**
1. **fork**（写锁全程）→ 引擎别 fork；要 fork 就 vfork/posix_spawn
2. **khugepaged collapse**（读锁+PTE 改写）→ THP=never 消灭
3. **运行期 mmap/munmap**（写锁）→ 布局启动期定格

排查工具：`/sys/kernel/debug/tracing/events/mm` 或 `bpftrace -e 'kprobe:__mmap_lock_acquire... '`；`perf lock`。

## 3. 引用计数双轨

| 计数 | 谁加减 | 归零后果 |
|------|--------|----------|
| `mm_users` | 线程创建/退出（CLONE_VM 共享不加）；`mmgrab` 类 | 归零 → 触发 exit_mmap（拆页表） |
| `mm_count` | `mmgrab()`：内核借用者（如 proc 读、AIO） | 归零 → mm_struct 本体释放 |

**经典坑：** 内核线程 `borrow` 用户 mm（lazy TLB，Ch3 §4）时 `mm_count++` 而非 `mm_users++`——借用不阻止 exit_mmap，只保证 mm_struct 本体不被释放。原书时代就有此双轨，v6.6 未变，面试常考。

## 4. 统计字段与观测

```bash
cat /proc/<pid>/status | grep -E 'VmPeak|VmSize|VmRSS|Threads'
cat /proc/<pid>/stat | awk '{print $24}'      # start_time 旁的 RSS 粗值
grep -w 'VmHWM' /proc/<pid>/status            # hiwater_rss：峰值驻留
```

| 字段 | 含义 | HFT 用途 |
|------|------|----------|
| `total_vm` | 虚拟总量 | 与预算比（预留地址空间可以大） |
| `RSS`/`hiwater_rss` | 实驻/峰值 | **锁死 mlock 后 RSS == 预算**，漂移=泄漏 |
| `data_vm` | 私有数据段 | 排除共享库干扰看私有增长 |

## 5. HFT / 嵌入式关联

| 主题 | 动作 |
|------|------|
| mmap_lock 争用 | 多线程读 maps/GUP 并发是读锁可忍；绝不允许运行期 mmap 类写锁事件 |
| fork 替代 | posix_spawn（fork+exec 合一、代价小）或纯线程模型 |
| mm 借用 | 内核线程与引擎同核时 borrow 会让页表 IPI 找上门（Ch3 §6 lazy TLB 代价） |
| 峰值监控 | hiwater_rss 告警线 = 预算×1.05，防慢泄漏 |

## 6. 衔接

- [§3 VMA](./section-3-内存区域.md)：mm_mt 里挂的东西
- [Ch 3](../../chapter-03-page-table-management/)：pgd 以下的世界
- [06.5/ch05 maple tree](../../../06.5-modern-mm/chapter-05-vm-address-space-maple-tree/)：树结构专项
- [05-linux-kernel 调度章](../../../05-linux-kernel/)：active_mm 借用的调度侧

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 mmap_lock 从 semaphore 改名 rw_semaphore？只是改名吗？**
A：语义统一到读写信号量（可 OPTIMIZE 的路径走读侧）。历史上 mmap_sem 是 rwsem，v5.8 前后统一命名并加了 lockdep/观测插桩（`mmap_lock_.*` tracepoint）。读码时旧名 mmap_sem 直接当 rwsem 读。

**Q2：两个线程同时缺页（不同 VA），会互相阻塞吗？**
A：不会。缺页只拿 **读锁**（VMA 树稳定即可），页表级的互斥由 **PTE 锁**（split ptlock，Ch3 §7）承担——每表页一把。锁粒度分层：mmap_lock（结构）→ pte lock（数据）。线程各 fault 各的页 = 各拿各的 pte 锁。

**Q3：`mm_users` 为 0 但 `mm_count` > 0，这个 mm 还能缺页吗？**
A：不能——没有线程用它（exit_mmap 已拆页表）。持有 mm_count 的是借用者（如异步 IO 的 ctx），只保留 `mm_struct` 元数据 **供回查**（比如 io_uring 完成时查原地址空间）。对象存在≠可用。

**Q4：`/proc/pid/maps` 读取会不会拖慢目标进程？**
A：拿读锁瞬间（拷 VMA 快照），但如果此刻有人持写锁（fork/munmap 中），读方要等。监控高频轮询 maps 在写锁事件多的进程上有放大效应——HFT 监控用 smaps_rollup（合并统计，锁窗口更短）。

**Q5：maple tree 比红黑树快在哪？量级多大？**
A：节点 16 槽（树高 ÷4 vs 二叉树）+ 节点内线性扫描（cache line 内）→ 查找访存次数少约一半。对 mmap 密集进程（JVM 数万 VMA）缺页/查找路径可测提升 10-30%。VMA 少的进程差异不显著——**数据结构选型的收益随规模非线性**。

</details>

---
