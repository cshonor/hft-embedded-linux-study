# Ch 4 进程地址空间 · Process Address Space

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读**（HFT：**mmap / mlock / 缺页路径** 与热路径强相关，建议精读 §3–§5）

虚拟内存的核心优势之一：**每个进程拥有独立的虚拟地址空间**。本章讲 Linux 如何 **描述、管理** 这些空间 — 从 **`mm_struct` / VMA** 到 **`mmap`/`munmap`/`mlock`**，再到 **缺页异常** 如何把「预留的 VA」变成「真正的物理页」。

> **时代说明：** 原书以 **32 位 x86（3G+1G）**、**2.6** 为主。 **x86_64** 为用户态提供巨大 canonical 地址空间，**用户/内核划分** 仍靠 **`PAGE_OFFSET`** 思路，但不再是「3GiB 用户 + 1GiB 内核」这一固定数字。

---

## 内核 vs 用户：两种「分配」哲学

| | **内核空间** | **用户空间** |
|---|-------------|-------------|
| **分配语义** | 请求后 **尽快** 得到物理页（或 kmalloc/vmalloc 路径） | 多数时候只是在 **线性地址里预留** VA 范围 |
| **何时有物理页** | 分配路径上 **立即**（或明确失败） | 往往 **首次访问**（读/写）才通过 **缺页异常** 真正分配 |
| **可见性** | 内核映射 **全局一致**（各进程共享内核页表上半） | **每进程独立** PGD / VMA，上下文切换时 **mm 可能变** |

**HFT 结论：** 热路径上 **第一次 touch 某页** = 可能 **page fault + 分配 + 清零/读盘** — 延迟尖刺。所以常用 **`mmap` + 预 touch**、**`MAP_POPULATE`**、**`mlock`**、**大页** 把 fault 挡在 **启动/预热阶段**。

→ 用户态 API：[08-TLPI](../08-The-Linux-Programming-Interface/) · 概念：[01-CSAPP Ch9](../01-CSAPP-3rd/chapter-09-virtual-memory/) · 内核对照：[05-LKD Ch15](../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-15-process-address-space/)

---

## 1. 线性地址空间 (Linear Address Space)

从内核视角，线性地址 **一分为二**：

```
  ┌────────────────────────────────────────────── 高地址
  │  内核空间 (Kernel Space)                       
  │  各进程共享同一套内核映射；上下文切换时 **不变**   
  ├──────────────── PAGE_OFFSET (如 0xC0000000) ─── 32-bit 分界
  │  用户空间 (User Space)                         
  │  **每进程独立**；上下文切换时随 **mm_struct** 改变
  └────────────────────────────────────────────── 低地址
```

| x86 32 位（原书典型） | 含义 |
|----------------------|------|
| **低 3GiB** | 用户空间 |
| **高 1GiB**（≥ `PAGE_OFFSET`） | 内核 **直接映射** 窗口（Ch 3 `__va`/`__pa`） |

**x86_64：** 用户态可用 **低 canonical 范围**（如 0–128TiB 量级，视配置而定）；内核在 **高位 canonical** — **思想相同**：**切换进程换用户页表，内核部分共享**。

---

## 2. 进程地址空间描述符 (`mm_struct`)

每个进程的 **完整地址空间** 由 **`mm_struct`** 统一管理：

| 要点 | 说明 |
|------|------|
| **线程共享** | **同一进程内线程** 通常 **共享一个 `mm_struct`**（`clone` 不带 `CLONE_VM`） |
| **页表根** | 指向该进程 **PGD**（Ch 3） |
| **VMA 组织** | **链表** 遍历 + **红黑树**（`mm_rb`）按地址快速查找 |
| **统计** | **RSS**（驻留集）、虚拟大小等 |
| **锁** | **`mmap_lock`**（读写锁）保护 VMA 与页表并发 |

```
task_struct ──► mm_struct ──► pgd
                  │
                  ├── VMA 链表 / mm_rb
                  └── rss, total_vm, …
```

→ Ch 1 推荐阅读路线 **第 4 步**：VMA 创建 — [`mm/mmap.c`](https://elixir.bootlin.com/linux/latest/source/mm/mmap.c)

---

## 3. 内存区域 (Memory Regions · VMA)

进程 **很少用满** 整个用户地址空间，而是 **稀疏** 使用若干 **段** — 每段一个 **`vm_area_struct` (VMA)**。

### VMA 是什么

| 属性 | 含义 |
|------|------|
| **一段连续 VA 范围** `[vm_start, vm_end)` | 不与其它 VMA **重叠** |
| **统一属性** | 相同 **权限**（读/写/执行）、相同 **用途**（堆、栈、mmap 文件、匿名映射、共享库…） |
| **操作函数** | `vm_ops` — 文件映射的 **fault/read** 等回调 |

**典型 VMA：** 可执行文件 **text**、**heap**、**stack**、**mmap** 的订单簿/共享内存、**vdso/vsyscall** 页（2.6+）。

### 相关系统调用（内核入口在 `mm/mmap.c` 等）

| 调用 | 作用 | HFT |
|------|------|-----|
| **`mmap`** | **创建** 新 VMA / 映射文件或匿名内存 | 订单簿 **预分配 arena**、**共享内存** |
| **`mremap`** | **移动/扩大** 已映射区域 | 动态扩容堆式缓冲区 |
| **`mlock` / `mlockall`** | **锁定** 页面于物理内存 — **禁止换出** | **延迟敏感进程标配** |
| **`munmap`** | **删除** 映射、释放 VMA |  teardown 时释放 |

**注意：** `mmap` 成功 **≠** 立刻占用等量 **物理 RAM** — 常只是 **VMA + 页表「洞」**；物理页在 **fault** 时落下。

---

## 4. 异常处理与缺页异常 (Page Faulting)

用户页 **未必常驻内存**。CPU 访问 **无有效 PTE** 或 **权限不符** 的地址 → **异常** → 内核 **`do_page_fault()`** → 架构无关 **`handle_mm_fault()`**。

### 合法缺页的三类核心场景

#### (1) 按需分配 (Demand Allocation) — **首次 touch**

| 页类型 | 读 | 写 |
|--------|----|----|
| **匿名页** | 可映射 **全局零页 (zero page)** — 读共享全 0 | **分配新物理页**，内容置 0（**COW 零页优化** 等后续演进） |
| **文件 / 设备映射** | 调 **`address_space` / 驱动** 从 **磁盘/后端** 读入页 | 可能先 **COW** 或 **分配 + 读盘** |

**HFT：** 冷路径 **prefault**（写遍每页） vs 生产 **首包 fault** — 必须选一种策略。

#### (2) 请求调页 (Demand Paging) — **从 swap 换回**

- PTE **not present**，但 **保存 swap 槽位号**
- fault 路径 **读 swap** → 填回物理页 → 更新 PTE present

**HFT：** **`mlock`** / **`swapoff`** / 足够 **RAM** — 避免热路径掉进 swap fault。

#### (3) 写时复制 (Copy-On-Write · COW) — **`fork()` 后写入**

| 步骤 | 行为 |
|------|------|
| **`fork()`** | 父子 **共享物理页**，PTE 标 **只读** |
| **一方写入** | **缺页** → 内核 **复制** 物理页 → 写者获 **私有副本**，打破共享 |

**HFT：** 多进程 **读共享** 订单簿快照可用；**写共享** 需 **`MAP_PRIVATE` COW** 或 **真共享 + 同步** — fork 后写大结构会 **突发 COW fault**。

### 缺页处理链（简图）

```
用户态访问 VA
    → MMU 异常
    → do_page_fault()
    → 查 VMA 合法？
         ├─ 否 → SIGSEGV
         └─ 是 → handle_mm_fault()
                    ├─ 首次分配 (demand alloc)
                    ├─ swap in (demand paging)
                    └─ COW break
    → 返回用户态重试指令
```

→ 接 Ch 3 **PTE 位**、Ch 10 **回收换出**、Ch 11 **swap**。

---

## 5. 内核与用户空间的数据拷贝

内核 **不能** 裸 dereference 用户指针 — 页可能 **未映射、已换出、仅用户可读**。

| API | 方向 |
|-----|------|
| **`copy_from_user()`** | 用户 → 内核 |
| **`copy_to_user()`** | 内核 → 用户 |
| **`get_user()` / `put_user()`** | 标量快捷版 |

**机制：**

1. 正常拷贝；若访问 **无效用户地址** → MMU 异常  
2. **异常表 (Exception Table)** 把 **出错 RIP** 映射到 **fixup 代码**  
3. fixup **返回错误码**，**不 oops 内核**

**HFT：**  syscall / **ioctl** 路径上的拷贝次数与 **用户指针校验** — 网关若 **内核模块** 或 **bpf** 边界，需理解 **为何不能直接用 `memcpy` 对用户指针**。

---

## 6. Linux 2.6 内核的新变化

| 变化 | 作用 |
|------|------|
| **vsyscall 页** | 部分 **系统调用** 快速路径（gettimeofday 等）— 减少 **软中断进内核** 成本；后演进 **vdso**（现代仍重要） |
| **4GiB/4GiB 分割补丁**（可选配置） | 用户/内核 **各 4GiB** — 32 位 **大内存用户进程** 场景 |
| **非线性 VMA / `MAP_POPULATE`** | **`mmap` 时预 fault** 填充页表；**`remap_file_pages`** 非线性文件映射 — **数据库 / 大文件随机访问** 优化 |

**HFT 现代对应：**

| 2.6 概念 | 今天 |
|----------|------|
| vsyscall | **vdso** — `clock_gettime` 用户态快速路径 |
| `MAP_POPULATE` | 仍可用 — **启动时换 latency 换确定性** |
| 非线性 mmap |  niche；HFT 更常用 **hugepage + 固定布局 arena** |

---

## 用户地址空间一图

```
进程
  mm_struct
    ├── PGD ──► 页表 (Ch 3)
    └── VMA[]  (vm_area_struct)
          ├── [exe text]
          ├── [heap]
          ├── [mmap: order book arena]  ← mmap/mlock
          └── [stack]

  访问某 VA
    → PTE miss → page fault → 分配/读盘/COW/swap-in
    → PTE hit  → 正常读写（TLB 仍可能 miss）
```

---

## HFT 精读 checklist

| 目标 | 手段 |
|------|------|
| **消除运行时 fault** | `mmap` + touch 全区域 / `MAP_POPULATE` / `mlockall` |
| **禁止 swap** | `mlock`、 cgroup 限制、足够物理内存 |
| **共享只读行情** | `MAP_SHARED` 只读映射；写用 **每进程副本** 或 **无锁 ring** |
| **避免 fork 后 COW 风暴** | `pthread` 共享地址空间；或 `MAP_SHARED` + 设计分离 |
| **syscall 延迟** | 理解 **vdso** vs 真实 syscall；减少 **copy_from_user** 体积 |
| **与 Ch 2–3 衔接** | fault 落下的是 **哪个 Zone 的 struct page**、PTE 如何 **present** |

---

## 相关章节

- 上一章：[chapter-03-页表管理.md](./chapter-03-页表管理.md)
- 下一章：[chapter-05-启动内存分配器.md](./chapter-05-启动内存分配器.md)
- 附录 D：[appendix-D-进程地址空间.md](./appendix-D-进程地址空间.md)
- Ch 1 阅读路线第 4 步 → 本章 VMA / [`mm/mmap.c`](https://elixir.bootlin.com/linux/latest/source/mm/mmap.c)

---
