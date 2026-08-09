## ④ 内存区域的链表与树

单个 `mm_struct` 可能含 **成百上千 VMA** — 内核用 **链表 + 红黑树** 双重索引：**遍历** vs **按地址查找**。

#### 两种结构

| 结构 | 字段 | 复杂度 | 用途 |
|------|------|--------|------|
| **单向链表** | **`mmap`** | O(n) 遍历 | **`/proc/maps` 输出**、**全扫描** |
| **红黑树** | **`mm_rb`** | O(log n) 查找 | **`find_vma`**、**page fault**、**mmap 冲突检测** |

```
mm_struct
    │
    ├── mmap ──► VMA_a ──► VMA_b ──► VMA_c ──► ...   （链表，按 vm_start 排序）
    │
    └── mm_rb （红黑树，按 vm_start 键）
              │
              └── 快速：给定 addr，找覆盖它的 VMA
```

#### 为何需要两种

| 操作 | 频率 | 结构 |
|------|------|------|
| **缺页 / `find_vma(addr)`** | **极高** | **红黑树** |
| **munmap 合并相邻 VMA** | 中 | 链表 + 树 **同步更新** |
| **调试 dump maps** | 低 | **链表 walk** |

#### 与 Ch 6 / Ch 4 同构

| 子系统 | 数据结构 |
|--------|----------|
| **VMA 管理** | **`mm_rb` 红黑树** |
| **CFS 调度** | **`vruntime` 红黑树** |
| **epoll / interval tree** | 其他 **树形索引** |

#### `mmap_cache` 优化（Ch 15.5）

| 事实 | 说明 |
|------|------|
| **局部性** | 连续 **fault** 常落在 **同一 VMA** |
| **`mmap_cache`** | 缓存 **上次 find_vma 结果** — **顺序访问** 快路径 |

#### 插入 / 删除不变量

| 不变量 | 原因 |
|--------|------|
| **VMA 不重叠** | 同一 addr **至多一个** VMA |
| **按 `vm_start` 排序** | **`find_vma` 语义** |
| **链表与树一致** | 增删 **同时维护** 两结构 |

**HFT：** 运行时 **VMA 数量应稳定** — **启动期 `mmap` 全部 ring**，盘中 **不再 munmap/mmap**（避免 **树 rebalance** 与 **锁**）。若用 **`MAP_FIXED`** 固定 VA，**重复映射** 仍触发 **`do_munmap` + 新建** — 应 **一次到位**。

→ [Ch 6 内核数据结构](../../chapter-06-kernel-data-structures/) · [Ch 4 CFS rbtree](../../chapter-04-process-scheduling/notes/section-4.3-Linux-调度算法.md) · [15.5 find_vma](./section-15.5-操作内存区域.md)

---
