# Ch 8 §1 Slab 分配器的三大核心目标

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 1. Slab 分配器的三大核心目标

| 目标 | 机制 | HFT 用户态镜像 |
|------|------|----------------|
| **消除内部碎片** | 小于一页的请求 **按对象大小** 从 slab 切分，而非整页浪费 | **object pool** 只分配 `sizeof(Order)` 对齐块，不 `malloc` 任意大小 |
| **缓存常用对象** | 释放的对象 **保持已初始化状态** 挂在 cache 上，避免重复 ctor/dtor | **free list 复用 Order** — reset 字段而非 `new/delete` |
| **优化硬件缓存（Slab 着色）** | 用 slab **剩余空间作 color 偏移**，使 **不同 slab 上同类型对象** 落到 **不同 cache line**，减 **false sharing / 行冲突** | **`alignas(64)`**、按 core 分 arena、padding 热字段 |

### Slab 着色 (Colouring) — 简图

```
同一 kmem_cache（对象大小固定）
  Slab A:  [obj][obj][obj]…  + color_offset_0  → 对象起始地址 mod cache_line = α
  Slab B:  [obj][obj][obj]…  + color_offset_1  →  mod cache_line = β  (α≠β)
```

**目的：** 多 CPU 同时访问 **不同 slab** 上的 **同类型对象** 时，少 **挤在同一条 cache line**。

---
