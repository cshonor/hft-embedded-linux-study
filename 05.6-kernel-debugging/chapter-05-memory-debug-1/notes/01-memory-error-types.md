# 5.1 内核内存错误的类型

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

内核内存错误是 HFT 系统最危险的 bug 类型——可能导致数据损坏、崩溃、安全漏洞。

## 内核内存错误分类

| 错误类型 | 说明 | 检测工具 | 危害 |
|---------|------|---------|------|
| **越界访问** (OOB) | 读写超出分配范围 | KASAN, KFENCE | 数据损坏 |
| **Use-After-Free** (UAF) | 释放后继续使用 | KASAN, KFENCE | 数据损坏/安全漏洞 |
| **Double Free** | 同一指针释放两次 | SLUB debug | 内存损坏 |
| **内存泄漏** | 分配不释放 | kmemleak | OOM |
| **未初始化使用** | 使用未初始化的变量 | KMSAN | 信息泄露 |
| **无效释放** | 释放非分配地址 | SLUB debug | 崩溃 |
| **踩踏** (Overwrite) | 覆盖相邻内存 | KASAN, SLUB debug | 数据损坏 |

## 内核内存分配器层次

```
kmalloc / vmalloc / alloc_pages
         ↓
    SLUB 分配器 (slab)
    ├── kmalloc-32 / 64 / 128 / 256 ... (通用 cache)
    └── 专用 cache (如 task_struct, inode_cache)
         ↓
    Buddy 分配器 (page)
         ↓
    物理页帧 (page frame)
```

## SLUB 分配器 (6.x)

| 特性 | SLAB (ULK3 时代) | SLUB (6.x 默认) |
|------|-----------------|----------------|
| 设计 | 复杂的 per-CPU 缓存 | 简化的 per-CPU partial |
| 结构体 | `struct kmem_cache` + `struct slab` | 简化 |
| 调试 | 内建 redzone/poison | 需 CONFIG_SLUB_DEBUG |
| 对象元数据 | 内嵌 | 可选外置 |
| per-CPU | array_cache | 单 freelist + partial |
| 多核扩展性 | 差（锁竞争） | 好（简化锁） |

## 内存错误示例

```c
// 1. 越界访问 (OOB)
char *buf = kmalloc(64, GFP_KERNEL);
buf[64] = 'x';  // 越界写 1 字节
// KASAN: slab-out-of-bounds

// 2. Use-After-Free
char *ptr = kmalloc(128, GFP_KERNEL);
kfree(ptr);
ptr[0] = 'y';  // 释放后访问
// KASAN: use-after-free

// 3. Double Free
char *p = kmalloc(32, GFP_KERNEL);
kfree(p);
kfree(p);  // 重复释放
// SLUB debug: double free detected

// 4. 内存泄漏
char *leak = kmalloc(1024, GFP_KERNEL);
// 忘记 kfree(leak)
// kmemleak: unreferenced object

// 5. 未初始化使用
int *val = kmalloc(sizeof(int), GFP_KERNEL);
if (*val == 42)  // 未初始化读取
// KMSAN: use of uninitialized value
```

## 错误检测工具对比

| 工具 | 检测类型 | 机制 | 开销 | 实时性 |
|------|---------|------|------|--------|
| KASAN | OOB/UAF/wild | 影子内存 | 2-3x | 即时 |
| KFENCE | OOB/UAF | 页保护 | ~1% | 即时（采样） |
| SLUB debug | OOB/double free | redzone/poison | 低 | 延迟（alloc/free时） |
| kmemleak | 泄漏 | 扫描引用 | 扫描时高 | 非实时 |
| KMSAN | 未初始化 | 影子内存 | 2-3x | 即时 |
| UBSAN | 整数溢出 | 编译器插桩 | 2-5% | 即时 |

## HFT 关联

HFT 自定义内核模块的内存错误 90% 是越界和 UAF：

1. **DMA buffer 越界**：网卡 DMA 写入超出分配的 buffer
2. **UAF**：释放 skb 后继续访问
3. **并发踩踏**：多核同时写共享数据结构

KASAN 能在开发期捕获大部分内存错误。生产环境用 KFENCE 做低开销采样检测。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** SLUB 为什么取代了 SLAB？

> SLAB 的 per-CPU 缓存设计复杂（array_cache + shared array_cache），在多核系统上锁争用严重且内存开销大。SLUB 简化了 per-CPU 缓存（只用一个 freelist + partial 链表），减少了元数据开销，在大规模多核系统上扩展性更好。

**Q2:** Use-After-Free 错误为什么在内核中特别危险？

> 释放的内存被 SLUB 回收并可能分配给其他模块。原模块继续访问该内存会读到/写入新模块的数据，导致数据损坏或安全漏洞（信息泄漏）。在内核中 UAF 可能导致权限提升（攻击者可以利用 UAF 覆盖函数指针）。

**Q3:** 内核内存错误的四种基本类型是什么？分别用什么工具检测？

> (1) 越界访问（out-of-bounds）→ KASAN redzone；(2) Use-After-Free → KASAN quarantine + KFENCE；(3) 未初始化使用 → KMSAN（Memory Sanitizer）；(4) 双重释放 → SLUB debug poison。

**Q4:** KASAN 和 SLUB debug 都能检测越界，有什么区别？

> SLUB debug 在对象周围插入 redzone 字节，只能在分配/释放时检查——是延迟检测。KASAN 通过影子内存实时检查每次访问——是即时检测。KASAN 更精确但开销更大。SLUB debug 开销较小但检测能力有限。

**Q5:** HFT 驱动中 DMA buffer 越界是什么场景？如何检测？

> 网卡 DMA 写入数据超出分配的 buffer 大小（如分配 1500 字节但收到 1600 字节的包）。KASAN 不直接检测 DMA（DMA 绕过 CPU 内存访问）。需要：(1) 确保 DMA buffer 大小足够；(2) 用 KASAN 检测驱动代码中对 buffer 的越界访问；(3) 用 KFENCE 检测 slab 对象的越界。

</details>

## 交叉引用

- [05.6 ch05 KASAN](../../chapter-05-memory-debug-1/notes/02-kasan.md)
- [05.6 ch05 SLUB debug](../../chapter-05-memory-debug-1/notes/04-slub-debug.md)
- [05.6 ch06 KFENCE](../../chapter-06-memory-debug-2/notes/01-kfence.md)
