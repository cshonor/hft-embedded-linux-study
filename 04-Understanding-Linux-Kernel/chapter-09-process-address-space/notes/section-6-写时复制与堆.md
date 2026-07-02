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
| VMA / 缺页专著 | [07 Gorman Ch 4](../../../05-Linux-Virtual-Memory-Manager/) |
| 大页 / mlock / NUMA | [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/) · [03 SysPerf Ch 7](../../../13-Systems-Performance-2nd/chapter-07-memory/) |

---

← [5. 请求调页](./section-5-请求调页.md) · 下一章 [Ch 10 系统调用](../chapter-10-system-calls/)
