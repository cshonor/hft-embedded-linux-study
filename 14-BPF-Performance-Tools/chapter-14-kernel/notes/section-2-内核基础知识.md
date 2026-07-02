# 2. 内核基础知识 (Kernel Fundamentals)

### 唤醒 (Wakeups)

线程 **阻塞** 离核（等 I/O、锁、futex…）→ 事件完成 → **另一上下文唤醒** 它。

| 概念 | 说明 |
|------|------|
| **唤醒链** | A 等 B，B 等 C… 形成依赖 |
| **观测价值** | 不只「在等什么」，还有 **「谁把我叫醒」** |

→ 工具：`wakeuptime`、`offwaketime`

→ LKD 调度/等待：[03-Linux-Kernel-Development Ch 4](../03-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-04-process-scheduling/)

### 内核内存分配

| 分配器 | 作用 |
|--------|------|
| **Slab/Slub** | 固定大小 **内核对象**（`kmalloc`、`kmem_cache`）— 缓存重用 |
| **页分配器** | **按页**（通常 4KiB）连续物理内存 `alloc_pages` |

| 用户态类比 | 内核 |
|------------|------|
| `malloc` | `kmalloc` / Slab |
| `mmap` 大块 | 页分配器 |

→ [05-Linux-Virtual-Memory-Manager](../05-Linux-Virtual-Memory-Manager/) · [Ch 7 用户态内存](../../chapter-07-memory/)

### 内核锁

| 类型 | 特点 |
|------|------|
| **自旋锁** | 忙等，不可睡眠 — 短临界区 |
| **mutex** | 混合：cmpxchg → 乐观自旋 → **睡眠** |
| **rwlock** | 读多写少 |
| **RCU** | 读无锁、延迟回收 — 网络/路径查找常见 |

**BPF：** `mlock`/`mheld` 针对 **mutex**；**自旋锁勿 kretprobe**（书中安全建议）— 用 **CPU profile** 找 spin 热点。

### Tasklets 与 Work Queues

| 下半部 | 说明 |
|--------|------|
| **Tasklet** | 软中断上下文，不可睡眠 |
| **Workqueue** | **内核线程** 执行，可睡眠 — 驱动耗时工作 |

→ 工具：**`workq`** 测 work handler 延迟

→ LKD 中断/下半部：[03-Linux-Kernel-Development Ch 7–8](../03-Linux-Kernel-Development/)

---
