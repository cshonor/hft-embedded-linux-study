## 5. 请求调页 (Demand Paging)

> **最后一刻** 才分配 RAM — 提高系统整体吞吐量

---

### 一、核心思想

| 传统 | Linux 请求调页 |
|------|----------------|
| `mmap` / 扩堆时立刻分配物理页 | 先建立 **VMA + 空页表项** |
| 浪费未访问的内存 | **访问时** 才分配物理页框 |

Ch 8 伙伴系统在此刻被调用，把物理页挂到进程页表。

---

### 二、匿名页与零页 (Zero Page)

**匿名区** — 不映射磁盘文件（堆、栈、匿名 `mmap`）：

| 操作 | `do_anonymous_page()` 行为 |
|------|---------------------------|
| **读** | 不立刻分配新物理页；页表项指向系统静态 **零页 (Zero Page)** — 全 0、只读、全局共享 |
| **写** | 零页不可写 → 缺页 → 分配 **专属物理页**，复制（或 COW 路径），标记可写 |

**效果：** 大量「分配但未写」的内存几乎 **零 RAM 成本**（如 `malloc` 后未 touch 的页）。

---

### 三、文件映射的对比

| 类型 | 缺页时 |
|------|--------|
| **匿名** | 零页 / 新分配页 |
| **文件映射** | 从 **页缓存** 或磁盘读入 — 常为 **Major Fault** |

---

### 四、HFT 启示

- **首次 touch** 触发缺页 — 延迟到交易路径上 = 抖动  
- 启动阶段 **`mmap` + 写一遍`** 或 **`mlock`** 把缺页摊到 warmup  
- 监控 **`/proc/vmstat` 的 pgfault / pgmajfault**

### 常见陷阱

1. 以为 `mmap` 后内存就分配好了——demand paging 下，物理页在首次访问时才通过 page fault 分配
2. 混淆 `MAP_POPULATE` 和 `mlockall`——前者预建页表（仍可被 swap），后者锁定物理页（不可 swap）
3. 在 swap 频繁的系统中运行 HFT——swap-in 是 major fault（毫秒级），必须禁用 swap

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** demand paging 的优势和代价？

<details><summary>答案</summary>

优势：① 节省物理内存（未访问的页不分配）。② 加快 `fork`/`exec`（不立即分配所有页）。③ 支持超量分配（overcommit）。代价：① 首次访问触发 page fault（~1-5us 延迟）。② 可能触发 swap（major fault，毫秒级）。③ 不可预测的延迟（HFT 大忌）。HFT 应在初始化时消除 demand paging：`MAP_POPULATE` + `mlockall` + 预 `memset`。

</details>

**Q2.** `overcommit_memory` 的三种模式？HFT 应该用哪个？

<details><summary>答案</summary>

0（启发式）：内核估算可用内存，可能拒绝大额分配。1（总是允许）：任何 `malloc` 都成功（直到 OOM kill）。2（严格）：`total_swap + total_ram * ratio` 限制。HFT 应设 `vm.overcommit_memory=2` + `vm.overcommit_ratio=90`：防止其他进程超量分配挤压 HFT 内存。同时 `swapoff -a` 禁用 swap。

</details>

**Q3.** 如何预填充所有页表消除 demand paging？

<details><summary>答案</summary>

```c
// 方法 1: MAP_POPULATE
void *p = mmap(NULL, size, PROT_READ|PROT_WRITE,
              MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE, -1, 0);
// 方法 2: 显式 touch 每页
char *p = mmap(...);
for (size_t i = 0; i < size; i += 4096)
    p[i] = 0;  // 触发 minor fault
// 方法 3: 大页 + MAP_POPULATE
mmap(NULL, size, ..., MAP_HUGETLB | MAP_POPULATE, ...);
mlockall(MCL_CURRENT | MCL_FUTURE);  // 锁定防 swap
```

</details>

</details>

---

← [4. 缺页异常](./section-4-缺页异常.md) · 下一节 [6. COW 与堆](./section-6-写时复制与堆.md)
> ↔ [LKD Ch15 §15.8 从访问到缺页概念](../../../05-linux-kernel/00_Book_3rd_Notes/chapter-15-process-address-space/notes/section-15.8-从访问到缺页概念.md)
