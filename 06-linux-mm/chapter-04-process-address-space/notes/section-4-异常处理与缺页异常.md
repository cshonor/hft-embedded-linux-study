# Ch 4 §4 异常处理与缺页异常（v6.6 fault 路径）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`mm/memory.c` :4067 `do_anonymous_page`、:3053 `wp_page_copy`、:3338 `do_wp_page`、:5255 `handle_mm_fault`）

---

## 本节讲什么

缺页是地址空间从"申请"到"兑现"的时刻——也是 HFT 延迟事件的高发地。本节把原书三类场景（demand alloc / swap in / COW）钉到 v6.6 的具体函数行号上，并补原书没有的第四类：**userfaultfd**。

---

## 1. 入口链（v6.6 实锚）

```
用户态访问 VA → MMU 异常
  → arch do_page_fault()（x86 exc_page_fault / ARM64 do_mem_abort）
  → 找 VMA（lockless 快路径 v6.6 per-fault VMA cache）
       无 VMA 且无 grow 栈可能 → SIGSEGV
  → handle_mm_fault(vma, addr, flags)        memory.c:5255
       → __handle_mm_fault(:5031)            逐级 alloc pgd/pud/pmd
       → pmd 是大页? → huge fault 路径
       → pte 级分四路（下）
```

## 2. 四路分发

### ① 按需分配（匿名首访）— `do_anonymous_page`（:4067）

| 访问 | v6.6 行为 |
|------|-----------|
| **读** | 映射 **全局零页**：`pte_mkspecial(pfn_pte(my_zero_pfn(...)))`（:4089 实锚）——不分配物理页！ |
| **写** | 分配新页（`alloc_anon_folio`，v6.6 已带 folio/多页批量）→ 零填充 → PTE 写权限 |

**零页共享的深意：** 大片"只读没写"的 .bss 类区域全指到同一张零页——RSS 几乎不涨。**调试技巧：** 想知道匿名段哪部分被写过，`/proc/pid/pagemap` 看哪些页仍指 zero_pfn。

**HFT：** 零页路径再快也是 fault（~µs）。prefault（写遍）一次交清，运行期零 fault——启动期 1 秒换运行期每秒上万次的确定性。

### ② 请求调页（swap in）

PTE 非空非 present → 解码 swap entry → `swapin_readahead`（预读邻页）→ PTE 恢复。
**HFT 消灭法：** mlock + 充足 RAM + `vm.swappiness=0`（防患）+ 监控 `pswpin/pswpout` 为零。

### ③ COW — `do_wp_page`（:3338）→ `wp_page_copy`（:3053）

```
fork 后写共享只读页：
  do_wp_page
    ├─ 只有一个映射且独占 → wp_page_reuse：直接改 PTE 可写（免拷贝！）
    └─ 共享中 → wp_page_copy:
         新页 = alloc + copy_user 高路径拷贝源页（:3082 __wp_page_copy_user）
         rmap 解开本进程旧页 → 挂新页 → PTE 指新页可写
```

| 细节 | 意义 |
|------|------|
| `wp_page_reuse` 免拷贝 | **fork 后子进程从未 touched 的页，父进程写回不复制**——refcount 归一即复用 |
| fork 拷贝的单位是 folio | v6.6 COW 以 folio（可多页）为单位判定复用/复制 |

**HFT fork 纪律的机制根据：** fork 后父进程写 1GiB → 最多 26 万次 COW fault（每次 alloc+copy+PTE+可能 flush）。要么纯线程模型，要么 fork 后 exec 前父进程冻结写。

### ④ userfaultfd（原书没有，v5.1+ 常用）

用户态进程接管缺页处理（`UFFDIO_REGISTER` 监视 VMA）：
- 用途：虚机/容器 **热迁移**（Post-copy：先迁映射，页访问时按需拉取）、CoW 快照（CRIU）、**分布式共享内存底层**
- `UFFDIO_COPY` 从另一进程直接灌页——**用户态可控的缺页分发器**
- HFT 相关性：跨机容灾行情镜像的潜在底座；日常少用但要知道缺页可被用户态接管

## 3. fault 的成本表（把 §1-§3 串起来）

| fault 类型 | 典型成本 | 组成 |
|-----------|----------|------|
| 匿名读（零页） | ~1µs | 异常+VMA 查+装 PTE |
| 匿名写 | ~1-2µs | +分配+零填充 |
| COW | ~2-5µs | +拷贝+rmap 调整 |
| 文件读 | 数十 µs~ms | +page cache miss 则磁盘 |
| swap in | **几十 µs~ms** | +块设备 |
| THP 缺页（成功） | ~几 µs | 大页 alloc |
| THP 缺页（触发 compaction） | **ms 级** | Ch6 §5 的慢路径 |

**对照用户的可见症状：** majflt 计数（`/proc/pid/stat` 第 10 字段后）非零 = 有文件/swap fault；minflt 高 = 匿名/零页。

## 4. 观测

```bash
/usr/bin/time -v ./engine         # minor/major page faults 汇总
perf stat -e page-faults,faults ./engine
bpftrace -e 'tracepoint:exceptions:page_fault_user { @[comm] = count(); }'
# 缺页原因细分（含 anon/cow/swap 归因）：
perf record -e 'mm_page_fault:*' ...
```

## 5. HFT checklist（本章核心出口）

| 目标 | 手段 | 验证 |
|------|------|------|
| 运行期零 fault | MAP_POPULATE+touch+mlock | 压测中 `page-faults` 增量为 0 |
| 无 swap | mlockall(MCL_CURRENT\|MCL_FUTURE) | pswpin/pswpout=0 |
| 无 COW 风暴 | 线程模型/写前 exec | minflt 稳定 |
| fault 若发生要可归因 | tracepoint 常备 | 06.7 工具链 |

## 6. 衔接

- [§3 VMA](./section-3-内存区域.md)：fault 判合法性的依据
- [Ch 3 §2](../../chapter-03-page-table-management/notes/section-2-遍历与使用页表.md)：fault 装的 PTE 语义
- [Ch 10/11](../../chapter-10-page-frame-reclamation/)：换出侧
- [06.5/ch06 folio](../../../06.5-modern-mm/chapter-06-page-cache-folio/)：文件 fault 的现代实现

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：匿名读缺页为什么不分配物理页？怎么发现这个"骗局"？**
A：零页优化——全 0 内容无需每进程一份。发现：`/proc/pid/smaps` 段 RSS 远小于映射大小；`pagemap` 显示 PFN 等于共享零页。破局：写一下就"现原形"（真分配）。**calloc 大区域后只读扫描 = 内存零消耗的假象**，benchmark 常见的坑。

**Q2：`wp_page_reuse` 什么条件下免拷贝？**
A：页的 mapcount/refcount 表明"只有我这一个映射者"（fork 的另一边已退出或从未写）→ 直接收回写权限，无需复制。所以 fork+exec 模型里，父进程 COW 成本趋近于零——真正贵的是 fork 后 **双方都活着且都写**。

**Q3：栈缺页自动增长是怎么回事？**
A：访问恰低于栈 VMA 起点的地址 → arch fault handler 判定在栈扩展范围内（RLIMIT_STACK + gap）→ `expand_stack()`：VMA 向下改 vm_start，然后走正常匿名缺页。**栈 VMA 是唯一"会动"的常规 VMA**——递归深的代码在高频切换线程上会反复扩展（每次是写锁事件）。

**Q4：userfaultfd 和 KVM 的 EPT 缺页什么关系？**
A：层级不同：EPT fault 是虚机物理地址层的（KVM 内部处理），userfaultfd 是进程 VA 层的。但虚机热迁移常 **两层数字配合**：EPT 把虚机内存标记为缺失 → 退出到 QEMU → QEMU 按需从源机拉页再 UFFD/THP 灌回。分布式 HFT 的行情热迁移若做，这套是参考架构。

**Q5：怎么区分"良性 prefault 期 fault"和"运行期异常 fault"？**
A：时间戳聚类：启动期集中爆发+之后归零=良性；稳态期持续零星=泄漏/越界（常见：越界写碰 VMA 边界、动态分配未池化）。bpftrace 给 fault 打时间戳直方图（`@usecs = hist(nsecs/1000)`）一眼可辨。

</details>

---
