# 5.1 内核内存错误的类型

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

### 内核内存错误分类

| 错误类型 | 说明 | 检测工具 |
|---------|------|---------|
| **越界访问** (OOB) | 读写超出分配范围 | KASAN, KFENCE |
| **Use-After-Free** (UAF) | 释放后继续使用 | KASAN, KFENCE |
| **Double Free** | 同一指针释放两次 | SLUB debug |
| **内存泄漏** | 分配不释放 | kmemleak |
| **未初始化使用** | 使用未初始化的变量 | KMSAN |
| **无效释放** | 释放非分配地址 | SLUB debug |
| **踩踏** (Overwrite) | 覆盖相邻内存 | KASAN, SLUB debug |

### 内核内存分配器层次

```
kmalloc / vmalloc / alloc_pages
         ↓
    SLUB 分配器 (slab)
         ↓
    Buddy 分配器 (page)
         ↓
    物理页帧 (page frame)
```

### SLUB 分配器 (6.x)

| 特性 | SLAB (ULK3 时代) | SLUB (6.x 默认) |
|------|-----------------|----------------|
| 设计 | 复杂的 per-CPU 缓存 | 简化的 per-CPU partial |
| 结构体 | `struct kmem_cache` + `struct slab` | 简化 |
| 调试 | 内建 redzone/poison | 需 CONFIG_SLUB_DEBUG |
| 对象元数据 | 内嵌 | 可选外置 |

### HFT 关联

HFT 自定义内核模块的内存错误 90% 是越界和 UAF，KASAN 能在开发期捕获大部分。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** SLUB 为什么取代了 SLAB？

> SLAB 的 per-CPU 缓存设计复杂（array_cache + shared array_cache），在多核系统上锁争用严重且内存开销大。SLUB 简化了 per-CPU 缓存（只用一个 freelist + partial 链表），减少了元数据开销，在大规模多核系统上扩展性更好。

**Q2:** Use-After-Free 错误为什么在内核中特别危险？

> 释放的内存被 SLUB 回收并可能分配给其他模块。原模块继续访问该内存会读到/写入新模块的数据，导致数据损坏或安全漏洞（信息泄漏）。在内核中 UAF 可能导致权限提升（攻击者可以利用 UAF 覆盖函数指针）。


**Q:** 内核内存错误的四种基本类型是什么？分别用什么工具检测？

> (1) 越界访问（out-of-bounds）→ KASAN redzone；(2) Use-After-Free → KASAN quarantine + KFENCE；(3) 未初始化使用 → KMSAN（Memory Sanitizer）；(4) 双重释放 → SLUB debug poison。

</details>

## 交叉引用

- [05.6 ch05 KASAN](chapter-05-memory-debug-1/notes/section-5-2.md)
- [05.6 ch06 KFENCE](chapter-06-memory-debug-2/notes/section-6-1.md)
