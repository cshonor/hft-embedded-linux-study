## 2. 内存描述符 (The Memory Descriptor)

> 进程地址空间的 **总账本** — `mm_struct`

---

### 一、`mm_struct` 包含什么

进程描述符 `task_struct` 的 **`mm`** 字段指向内存描述符，记录整个用户地址空间：

| 字段类 | 内容 |
|--------|------|
| **`pgd`** | 页全局目录指针 — 进程私有页表根 |
| **段界** | 代码段、数据段、堆、栈的 **起止线性地址** |
| **VMA 集合** | 所有 `vm_area_struct`（链表 + 红黑树） |
| **堆界** | `start_brk`、`brk`（见 [section-6](./section-6-写时复制与堆.md)） |

→ 页表层次：[Ch 2 四级页表](../../chapter-02-memory-addressing/notes/section-4-四级页表.md)

---

### 二、引用计数：`mm_users` vs `mm_count`

| 计数器 | 含义 |
|--------|------|
| **`mm_users`** | 共享该地址空间的 **轻量级进程（线程）** 数量 |
| **`mm_count`** | 内存描述符本身的 **主引用计数** |

线程组内多线程 **共享同一 `mm_struct`**；最后一个引用释放时才销毁地址空间。

→ 线程与 LWP：[Ch 3 section-1](../../chapter-03-processes/notes/section-1-本章定位.md)

---

### 三、内核线程的特殊性

| 对象 | `mm` 字段 |
|------|-----------|
| **普通进程 / 用户线程** | 指向有效 `mm_struct` |
| **内核线程** | **`NULL`** — 不拥有用户地址空间 |

内核线程被调度时：

- **借用** 前一个用户进程的 `mm`  
- 存于 `active_mm` — 仅用于访问用户页表等内核路径需要  

### 常见陷阱

1. 混淆 `mm_struct` 的引用计数——`mm_count` 是 `mm_struct` 本身的引用，`mm_users` 是使用该 mm 的线程数
2. 以为内核线程没有 `mm_struct`——内核线程 `mm = NULL`，但 `active_mm` 借用前一个用户进程的
3. 在持有 `mmap_lock` 时做耗时操作——`mmap_lock` 是读写锁，写锁会阻塞所有 `page fault`

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `mm_users` 和 `mm_count` 的区别？

<details><summary>答案</summary>

`mm_users`：使用该地址空间的线程数（`clone(CLONE_VM)` 共享 mm 时 +1）。降到 0 时释放 `mm_struct` 的用户资源（页表、VMA）。`mm_count`：`mm_struct` 本身的引用（内核模块/`active_mm` 持有）。降到 0 时释放 `mm_struct` 结构体本身。关系：`mm_users` 每次降为 0 时 `mm_count` -1。所以 `mm_users > 0` 时 `mm_count >= 1`。

</details>

**Q2.** `mmap_lock`（原 `mmap_sem`）在 6.x 内核中有什么变化？

<details><summary>答案</summary>

① 改名 `mmap_sem` → `mmap_lock`（更准确表达它是 lock 不是 semaphore）。② 从 RWSEM 改为可配置的 `rw_semaphore`。③ 新增 `mmap_read_trylock()` / `mmap_write_trylock()` 非阻塞接口。④ `page fault` 路径用 `mmap_read_lock()`（读锁，不阻塞其他 fault）。⑤ `mmap`/`munmap`/`mprotect` 用 `mmap_write_lock()`（写锁，阻塞所有 fault）。HFT 热路径避免 `mprotect`（会持写锁阻塞 fault）。

</details>

**Q3.** 如何查看进程的 `mm_struct` 状态？

<details><summary>答案</summary>

① `/proc/[pid]/status` 中的 `VmPeak`/`VmSize`/`VmRSS`/`VmData`/`VmStk`/`VmExe`。② `/proc/[pid]/maps`：所有 VMA。③ `/proc/[pid]/smaps`：每个 VMA 的详细统计（RSS、PSS、anon、swap）。④ `/proc/[pid]/statm`：页级别统计。⑤ `pmap [pid]`：格式化输出。HFT 诊断内存问题用 `smaps` 看每个映射的 RSS 和大页状态。

</details>

</details>

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 内存区 VMA](./section-3-内存区VMA.md)
> ↔ [LKD Ch15 §15.2 内存描述符](../../../05-linux-kernel/chapter-15-process-address-space/notes/section-15.2-内存描述符.md)
