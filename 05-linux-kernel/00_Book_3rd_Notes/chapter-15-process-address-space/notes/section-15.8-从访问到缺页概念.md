## ⑧ 从访问到缺页概念

**缺页（page fault）** 不一定是错误 — 常是 **按需分配**、**COW**、**读文件 backing** 的 **正常机制**。HFT 目标：**热路径零 fault**。

#### 完整路径（概念）

```
用户/内核读写 VA
        ▼
   MMU 查 TLB
        ├─ hit ──────────────────────► 访问 PA
        └─ miss ──► 页表 walk
                      ├─ PTE present ──► 填 TLB ──► 访问 PA
                      └─ PTE 不在 / 权限错 ──► #PF 异常
                                ▼
                        page fault handler (arch)
                                ▼
                        do_page_fault() / handle_mm_fault()
                                ├─ find_vma(addr)
                                ├─ 权限检查 (SEGV?)
                                └─ __handle_mm_fault()
                                      ├─ 匿名：alloc_page + 填 PTE
                                      ├─ 文件：page cache / readpage
                                      ├─ COW：复制页 + 改 PTE writable
                                      └─ swap：swapin（慢）
```

#### fault 类型

| 类型 | 典型原因 | 代价 |
|------|----------|------|
| **minor fault** | **首次 touch 匿名**、**读已缓存文件页** | **中** — 仍进内核 |
| **major fault** | **读盘**、**swap in** | **极高** — ms 级 |
| **COW fault** | **`fork` 后首次写** | **中** — 复制页 |
| **SIGSEGV** | **无 VMA**、**PROT 不符** | 进程终止 |

#### 与 Ch 3 / Ch 12 / Ch 16 接线

| 场景 | 处理 |
|------|------|
| **`fork`** | **复制 VMA**；PTE **只读共享** → 写时 **COW**（Ch 3） |
| **匿名堆增长** | **`handle_mm_fault`** → **`alloc_pages`**（Ch 12） |
| **文件 mmap 读** | **页缓存 hit** → minor；**miss** → **block IO**（Ch 16） |
| **`mlock` 范围** | fault 后页 **标记不可换出** |

#### `handle_mm_fault` 四步（Gorman 对齐）

| 步 | 名 | 行为 |
|----|-----|------|
| 1 | **find_vma** | 定位 VMA |
| 2 | **access check** | 写只读？→ SIGSEGV |
| 3 | **`alloc_pte`** | 必要时 **分配中间页表** |
| 4 | **`handle_pte_fault`** | 建 PTE / COW / swap |

#### 用户态等价与规避

| 内核 fault 原因 | HFT 规避 |
|-----------------|----------|
| **首次 touch** | **`MAP_POPULATE`**、**`touch pages` 启动循环** |
| **swap** | **`mlock` / `vm.swappiness=0`** |
| **THP collapse** | **`madvise(MADV_NOHUGEPAGE)`** 或 sysctl |
| **COW after fork** | **`MADV_DONTFORK`**、**不再 fork** |
| **文件 mmap miss** | **预热读**、**tmpfs/hugetlb** |

#### 测量

| 工具 | 指标 |
|------|------|
| **`perf stat -e page-faults,major-faults`** | fault 计数 |
| **`/proc/self/status` VmRSS / VmLck** | 常驻 / 锁定 |
| **`time` 首触大映射** | **major fault 尖刺** |

**HFT：** 把 **缺页当作 bug**（热路径）— **启动阶段故意承担** 所有 fault + **mlock**，盘中 **profile 应 page-faults ≈ 0**。一次 **major fault** 在 **微秒策略** 里 = **灾难**。

→ [Ch 3 COW](../../chapter-03-process-management/) · [Ch 12 页分配](../../chapter-12-memory-management/) · [06 Gorman 缺页异常](../../../../06-linux-mm/chapter-04-process-address-space/notes/section-4-异常处理与缺页异常.md) · [Ch 16 页缓存](../../chapter-16-the-page-cache-and-page-writeback/)


> ↔ [ULK Ch9 §5 请求调页](../../../../20-linux-kernel-deep/chapter-09-process-address-space/notes/section-5-请求调页.md)
---
