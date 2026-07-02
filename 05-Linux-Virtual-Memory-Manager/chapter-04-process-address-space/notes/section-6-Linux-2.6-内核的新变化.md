# Ch 4 §6 Linux 2.6 内核的新变化

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 6. Linux 2.6 内核的新变化

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
