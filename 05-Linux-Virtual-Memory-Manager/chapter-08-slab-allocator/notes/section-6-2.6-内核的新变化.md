# Ch 8 §6 2.6 内核的新变化

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 6. 2.6 内核的新变化

### 内存池 `mempool`

**`mempool_t`** — 在 **系统极度缺内存** 时仍 **保留最低限度关键对象**（如 **bio、scsi 结构**）— **预分配 + reserve list**，**GFP 失败也不死**。

**HFT：** 交易 **reserve order slot**、**预分配 cancel 队列深度** — 同一 **「内存压力下仍要能完成关键路径」** 逻辑。

### 缓存回收：shrinker 取代 `kmem_cache_reap()`

| 2.4 及更早 | 2.6+ |
|------------|------|
| **`kmem_cache_reap()`** 盲目全局 reap | **`set_shrinker()`** — cache **注册自定义 shrink 回调** |
| 粗暴 | **按压力、按类型** 智能释放 **slabs_free** 回 Buddy |

现代 **`register_shrinker()`** / **`shrink_slab`** — 与 **kswapd / memcg** 联动。

---

## Buddy → Slab → kmalloc 一图

```
         应用 / 内核子系统
                │
    ┌───────────┼───────────┐
    │           │           │
 kmem_cache_alloc   kmalloc    get_free_pages
 (专用类型)      (任意小尺寸)   (整页)
    │           │           │
    └───────────┴───────────┘
                │
           Slab / SLUB
      full / partial / free
      per-CPU object cache
                │
           Buddy (Ch 6)
```

---

## HFT 精读 checklist

| Slab 概念 | 用户态落地 |
|-----------|------------|
| **专用 cache** | 一种消息/订单 **一种 pool** |
| **partial 优先** | 先 **pop free list**，空了再 **grow** |
| **constructor** | pool 分配时 **一次初始化**，复用时 **reset 热字段** |
| **着色 / 对齐** | **64B align**，热冷数据 **分 cache line** |
| **per-CPU array** | **每核 mempool**，批量 refill |
| **mempool reserve** | **预留 N 个 slot** 给 **风控/撤单** 关键路径 |
| **kmalloc 式泛型分配** | 热路径 **禁止** — 用 **固定 size pool** |

---
