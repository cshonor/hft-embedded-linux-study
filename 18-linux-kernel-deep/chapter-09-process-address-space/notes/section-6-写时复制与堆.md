## 6. 写时复制 (COW) 与堆管理

---

### 一、COW 解决什么问题

传统 Unix **`fork()`** 完整复制父进程地址空间 → **极慢**，且子进程常立刻 **`exec()`**，复制纯属浪费。

Linux **写时复制**：

| 阶段 | 行为 |
|------|------|
| **`fork` 后** | 父子 **共享** 相同物理页；页表项标 **只读** |
| **任一方写入** | 缺页 → **`do_wp_page()`** 分配新物理页、复制内容、改为可写 |

→ fork 路径预告：[Ch 3 section-6](../../chapter-03-processes/notes/section-6-创建与销毁.md)

---

### 二、`do_wp_page()` 要点

1. 确认是 **合法写** 共享只读页  
2. 若仍有多引用 → **分配新页框** + 拷贝 + 更新页表  
3. 若仅单引用 → 可直接 **取消只读** 标记（无需复制）

COW 同时服务于 **fork** 与 **零页首次写入** 等路径。

---

### 三、堆 (Heap) 管理

每个 Unix 进程有 **堆** VMA，供 **`malloc`**（经 libc）动态分配：

| 字段 / 接口 | 作用 |
|-------------|------|
| **`start_brk`** | 堆起始线性地址 |
| **`brk`** | 堆当前结束地址（可动态变化） |
| **`brk()` 系统调用** | 用户直接调整堆大小 |
| **`sys_brk()`** | 内核服务：扩大 → **`do_mmap()`**；缩小 → **`do_munmap()`** |

现代 **`malloc`** 多数先用 **`brk` 扩堆**，大分配走 **`mmap`** 独立 VMA。

→ 系统调用层：[Ch 10 System Calls](../../chapter-10-system-calls/) · TLPI

---

### 四、本章小结

```
mm_struct（地址空间总账）
    ↓
vm_area_struct（权限相同的线性区间）
    ↓
do_page_fault（非法 → SIGSEGV；合法 → 调页/COW）
    ↓
Ch 8 物理页分配
```

**Ch 8 物理页 + Ch 9 虚拟地址** 在此完整衔接。

---

### 五、后续章节索引

| Ch 9 主题 | 继续读 |
|-----------|--------|
| brk / mmap  syscall | [Ch 10 系统调用](../chapter-10-system-calls/) 🔴 |
| fork / COW 创建路径 | [Ch 3 创建与销毁](../chapter-03-processes/notes/section-6-创建与销毁.md) 🔴 |
| 页表 / TLB | [Ch 2 内存寻址](../chapter-02-memory-addressing/) 🔴 |
| 物理页分配 | [Ch 8 内存管理](../chapter-08-memory-management/) 🔴 |
| 页回收 / swap | [Ch 17 页回收](../chapter-17-page-reclaim.md) 🟡 |
| VMA / 缺页专著 | [07 Gorman Ch 4](../../../06-linux-mm/) |
| 大页 / mlock / NUMA | [16 HFT 工程](../../../16-hft-engineering/) · [03 SysPerf Ch 7](../../../14-systems-performance/chapter-07-memory/) |

### 常见陷阱

1. 以为 COW 是零开销——COW 首次写触发 page fault + 分配新物理页 + 复制内容，开销 ~1-5us/页
2. 混淆 `brk()` 和 `mmap()`——`brk()` 扩展堆（连续增长），`mmap()` 分配独立 VMA（可任意位置）
3. 在 HFT 中频繁 `malloc`/`free`——glibc malloc 可能调用 `brk`/`mmap` 系统调用，引入延迟

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** COW 的完整流程？fork 后子进程写页时发生什么？

<details><summary>答案</summary>

① `fork()` → `copy_page_range()`：复制 PTE，所有 PTE 设为只读，物理页引用计数 +1。② 子进程写某页 → CPU 触发 #PF（写只读页）。③ `do_wp_page()`：分配新物理页，复制旧页内容，子进程 PTE 指向新页（可写），旧页引用计数 -1。④ 如果旧页引用计数降为 0，旧页被回收。COW 延迟 = 1 page fault + 1 alloc + 1 memcpy ≈ 2-5us/页。

</details>

**Q2.** `brk()` 和 `mmap()` 在堆管理上的区别？

<details><summary>答案</summary>

`brk(addr)`：设置 program break（堆顶），堆是连续的 VMA，`malloc` 小对象用 `brk`（快、局部性好）。`mmap(NULL, size, ...)`：在 mmap 区创建独立 VMA，`malloc` 大块（>128KB）用 `mmap`（避免堆碎片）。`brk` 只能收缩/扩展堆，`mmap` 可任意分配/释放。glibc `malloc` 默认：小对象→`brk`，大对象→`mmap(MAP_ANONYMOUS)`。HFT 应预分配内存池，避免运行时 `malloc`。

</details>

**Q3.** HFT 如何避免 `malloc`/`free` 引起的延迟？

<details><summary>答案</summary>

① 预分配内存池：启动时 `malloc` 所有需要的内存，运行时从池中分配（无系统调用）。② `mallopt(M_MMAP_THRESHOLD, ...)` 调大 `brk` 阈值，减少 `mmap` 调用。③ `LD_PRELOAD=libjemalloc.so` 用 jemalloc 替代 glibc malloc（更低的碎片和锁竞争）。④ 完全自管理：`mmap` 一大块 + 自实现 free list。⑤ `malloc_trim(0)` 归还碎片（但会引起 `brk` 系统调用）。

</details>

</details>

---

← [5. 请求调页](./section-5-请求调页.md) · 下一章 [Ch 10 系统调用](../chapter-10-system-calls/)
