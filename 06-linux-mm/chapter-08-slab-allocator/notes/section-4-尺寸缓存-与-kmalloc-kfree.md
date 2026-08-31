# Ch 8 §4 尺寸缓存与 `kmalloc` / `kfree`（v6.6 档位全表）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`mm/slab_common.c:824` kmalloc_info[]、`include/linux/slab.h:364` KMALLOC_TYPES）

---

## 本节讲什么

`kmalloc` 是内核的 `malloc`：任意小尺寸请求 → 选最近档位 cache → slab 分配。本节给 v6.6 的**真实档位表**（原书是 2.4 的 size-N 系列）、选档算法、以及"为什么 struct 尺寸对齐能白赚 30% 吞吐"的量化依据。

---

## 1. v6.6 档位表（slab_common.c:824 实锚）

```
8 · 16 · 32 · 64 · 96 · 128 · 192 · 256 · 512 · 1k · 2k · 4k · 8k ·
16k · 32k · 64k · 128k · 256k · 512k · 1M · 2M
```

| 特征 | 说明 |
|------|------|
| 2 的幂为主体 | 8→2M |
| **两个非 2 幂插档：96、192** | 吸收"64+header""128+header"型请求——C struct 常见尺寸的密度峰 |
| 上限 2M | 超过直接走 buddy（`kmalloc_large`→alloc_pages） |
| 每档多个类型副本 | `kmalloc_caches[NR_KMALLOC_TYPES][shift]`（slab.h:388）二维数组 |

**选档算法：** size ≤ 192 查 `size_index[]` 表（O(1)，96/192 两档就在这张表里）；>192 上取 2 幂。

## 2. 类型维度（KMALLOC_TYPES，slab.h:364）

| 类型 | 用途 |
|------|------|
| `KMALLOC_NORMAL` | 默认 |
| `KMALLOC_DMA` | `GFP_DMA` 请求（ISAL 低端内存，现代 x86_64 少用） |
| `KMALLOC_CGROUP` | 有 memcg 记账的请求（slab 计费入组，5.9+ root/非 root 分列） |
| `KMALLOC_RECLAIM` | `__GFP_RECLAIMABLE`（可回收 slab，回收器优先收缩它们） |

**为什么分类型副本而不是每对象打标：** 同一 slab 里混记账/不记账对象会让 memcg 统计与回收逻辑复杂化；**按"记账属性"分池**让 slab 整块同质——用户态池设计的同款决策（区分可回收/不可回收 arena）。

## 3. 内部碎片量化

| 请求 size | 落档 | 浪费 |
|-----------|------|------|
| 64 | 64 | 0% |
| 65 | 96 | 32% |
| 97 | 128 | 24% |
| 200 | 256 | 22% |
| 513 | 1k | 44% |

**v6.6 新利好：`kmem_cache_create_usercopy` 与 `kmalloc` 的差距在缩小**——但 96/192 两档的存在就是内核承认"常见 struct 尺寸不长在 2 幂上"的证据。**HFT 规则：热路径 struct 尺寸要么压到 2 幂（64/128），要么就是池化专用 cache 完全绕开 kmalloc。**

## 4. `kfree` 怎么知道对象属于哪个 cache？

`kfree(ptr)` 不接收 cache 参数——答案在 **页元数据**：

```
ptr → virt_to_head_page(ptr) → struct page(folio)
     → folio_slab() 判定是 slab 页
     → page->slab_cache 指回 kmem_cache
```

所以 kmalloc 的对象 **天然不能跨界释放**（kfree 一个 vmalloc 指针/栈指针 = 未定义）。这也解释了 §2 为什么 `struct slab` 寄生在 page 上：**free 路径必须能从裸指针反查归属**——用户态池同理（arena header 或 size-class 索引位）。

## 5. kmalloc 家族速查

| API | 场景 |
|-----|------|
| `kmalloc(size, gfp)` / `kfree` | 通用小对象 |
| `kmalloc_node(size, gfp, nid)` | NUMA 指定节点（per-CPU 结构、驱动 ring） |
| `kzalloc` | + memset（零页回收友好） |
| `kmalloc_array` / `kcalloc` | 数组（带溢出检查） |
| `kmem_cache_alloc(s, gfp)` | 专用池（热路径首选） |
| `kmem_cache_alloc_bulk` | 批量（一次 CAS 拿 N 个，slub.c:3915 附近 free_bulk 对称） |
| `devm_kmalloc` | 设备生命周期绑定（驱动支线） |

**`kmem_cache_alloc_bulk` 值得注意：** 批量接口把 N 次快路径合并成 1 次（一次慢路径换回一批），高频小对象场景（网络栈、块层）专用——**用户态池的 batch pop 同构**。

## 6. 原书对照

| 原书（2.4） | v6.6 |
|--------------|------|
| size-N 系列（32B 起） | 21 档 + 4 类型副本 |
| size-N(DMA) | KMALLOC_DMA（同思想） |
| 无记账 | KMALLOC_CGROUP / memcg slab 计费 |
| `kmalloc(>128K)` 封顶 | 封顶 2M，超走 buddy |

## 7. HFT / 嵌入式关联

| 实践 | 依据 |
|------|------|
| 热路径 struct 定长对齐 2 幂 | 档位表 0% 浪费行 |
| 订单/消息对象走专用 kmem_cache（内核态）或对象池（用户态） | 绕开选档+记账 hook |
| 观测内核态开销：`/proc/slabinfo` 按大小排序 | 找出 alloc 热的 cache（skb_data、kmalloc-512 常客） |
| 内存压力下 kmalloc 失败处理 | GFP_ATOMIC 返回 NULL 必须处理——HFT 风控路径不能假设分配必成 |

## 8. 衔接

- [§3 对象分配与释放](./section-3-对象分配与释放.md)：选好档后的瀑布
- [Ch 6 物理页分配](../../chapter-06-physical-page-allocation/)：>2M 的去向
- [06.5/ch02](../../../06.5-modern-mm/chapter-02-slab-slub-allocator/)：memcg slab 计费的现代细节

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 96 和 192 两个"怪档"能活到今天？**
A：64B 和 128B 是 struct 密度峰（带一两个指针/计数器的业务结构），加 8~64B 头（debug/kasan）就溢出到下一档。插 96/192 两档后，这类请求浪费从 ~30% 降到 ~0~15%。这是 1999 年的 profiling 结论沿用至今——**尺寸档位是负载画像的化石**。

**Q2：`kmalloc(0)` 返回什么？**
A：返回 `ZERO_SIZE_PTR`（(void*)16），kfree 它也合法。用户态别模仿——返回唯一哨兵值省一次分配，但调用方忘判就是野指针。内核侧能这么做是因为 kfree 侧同样识别哨兵。

**Q3：`kmalloc(3M)` 会失败吗？**
A：不会走 slab——超 MAX_ORDER 相关上限时转 `kmalloc_large` → alloc_pages 高阶分配。3M 需要连续阶数高（order≥10），碎片化机器上高阶分配可以失败（返回 NULL）或触发 compaction。**需要大缓冲请用 vmalloc（非连续）或专用页池。**

**Q4：kfree 一个 kmalloc 返回的指针中间位置（ptr+16）会怎样？**
A：`virt_to_head_page` 仍指到同一页、同一 cache——free 进错误对象槽位，freelist 被污染 = 延迟爆炸/内存腐坏。这就是 CONFIG_DEBUG_SLAB/KFENCE 要抓的一类 bug。用户态 free 错指针同理，是 pool API 设计成"句柄化"（pool_index 而非裸指针）的动机。

**Q5：怎么快速判断一次内核路径分配走哪个档？**
A：`bpftrace -e 'kprobe:kmem_cache_alloc /comm=="myproc"/ { @[((struct kmem_cache *)arg0)->name] = count(); }'` 按调用者聚合 cache 名；或 `kprobe:__kmalloc` 看入参 size 分布。06.7 的 BPF 工具集（`kmem` 系列）已封装好这类查询。

</details>

---
