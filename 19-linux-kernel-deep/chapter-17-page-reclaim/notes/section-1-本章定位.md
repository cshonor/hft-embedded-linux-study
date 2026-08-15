## 1. 本章定位

> **ULK Ch 17 Page Frame Reclaiming** · 物理内存 **不够用时怎么办**

---

### 一、本章讲什么

RAM 有限 — 内核必须在 **响应速度** 与 **满足分配** 之间平衡：

| 机制 | 作用 |
|------|------|
| **PFRA** | 页框回收算法 — 选页、同步、释放 |
| **反向映射** | 共享页 — 清空 **所有 PTE** |
| **LRU** | 区分 **活跃 / 非活跃** — 先回收「久未用」 |
| **kswapd** | 后台 **周期性** 回收 |
| **Swap** | 匿名页等 **无磁盘映像** → 换出到交换区 |
| **OOM Killer** | 实在回收不动 → **杀进程** 释内存 |

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-PFRA与页分类.md) | 四类页、PFRA 原则 |
| [3](./section-3-反向映射.md) | anon_vma、PST |
| [4](./section-4-LRU链表.md) | active/inactive、PG_* 标志 |
| [5](./section-5-执行时机与OOM.md) | try_to_free_pages、kswapd、OOM |
| [6](./section-6-交换机制.md) | swap 区、swap cache、swap token |

---

### 三、在 Linux 链上的位置

```
Ch 8  物理页分配（伙伴系统）
Ch 9  匿名页、文件映射页
Ch 15 页缓存（可同步回收）
Ch 17 页框回收（本章）
Ch 9  换入时缺页异常
```

HFT：**mlock / 大页 / NUMA 本地 / 预留内存** — 避免热路径页被回收或 **swap 颠簸**。

---

← [Ch 17 导读](../README.md) · 下一节 [2. PFRA](./section-2-PFRA与页分类.md)
