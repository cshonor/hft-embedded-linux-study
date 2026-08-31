# Ch 8 §6 2.6 内核的新变化 → v6.6 演进（mempool / shrinker / slab 全家）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`mm/mempool.c`、`mm/shrinker.c`、`mm/slub.c`）

---

## 本节讲什么

原书章末"What's New in 2.6"讲了 mempool 与 shrinker 两个仍然活着的机制。本节把它们接到 v6.6，再补 slab 生态二十年沉淀的 debug/安全/记账全家桶——这张全家桶清单是"内核态组件选型"的菜单。

---

## 1. mempool：内存压力下的保证语义

**`mempool_t`** = 预分配 reserve + 借还记账，核心承诺：**池未耗尽时分配必成**（GFP 失败也回退到 reserve）。

| 场景 | v6.6 用户 |
|------|-----------|
| 块层 bio | `bioset`（mempool 的批量版） |
| scsi/target | command pool |
| 写回 | b_io 页数组 |

```
mempool_alloc(pool, gfp)
  ├─ 正常 alloc（进 shrink 后可能失败）
  ├─ 失败 → 从 pool->elements 弹预分配元素
  └─ 允许等待（pool->free 收到归还再给你）── WARN 点：持锁等待 = 死锁温床
```

**HFT 镜像：** 交易引擎的 **reserve cancel slot**——内存再紧，撤单路径必须能分配。语义三件套照抄：① 预留量与峰值压力挂钩；② 正常态不动用预留；③ 用尽时的等待必须有超时与告警。

## 2. shrinker：从 kmem_cache_reap 到 注册式回收

| 2.4 `kmem_cache_reap()` | 2.6+ `register_shrinker()` → v6.6 |
|--------------------------|-----------------------------------|
| 盲扫全局 cache 收 free slab | **cache 自注册回调**，按压力/按对象类型收缩 |
| 粗粒度 | 与 kswapd、direct reclaim、memcg 三方联动 |
| 无优先级 | `shrinker->seeks` 权重 + count_objects/scan_objects 两段式 |

v6.6 的 slab shrinker 已是 **通用缓存收缩协议**（dentry/inode/xfs/zram/slab 全用同一接口）。THP 的 `deferred_split_shrinker`（huge_memory.c:69）也是它——Ch 3 的延迟拆大页队列就挂在这个框架上。

## 3. slab 生态全家桶（v6.6 选型菜单）

| 机制 | CONFIG / 接口 | 作用 | 开销 | HFT 建议 |
|------|---------------|------|------|----------|
| freelist 随机化 | `SLAB_FREELIST_RANDOM` | 防堆布局预测 | 建池时一次 | 开 |
| freelist 加固 | `SLAB_FREELIST_HARDENED` | 指针 XOR + 越界校验 | free 路径少量算术 | 生产开 |
| **KFENCE** | `KFENCE` | 采样式 use-after-free 抓捕（每 slab 页陷阱） | 采样命中才有 ~100ns | 调试期开 |
| **KASAN** | `KASAN` | 影子内存全量检查 | **每次 kmalloc +~15-20% CPU** | 生产**关** |
| memcg slab 计费 | `MEMCG_KMEM` | slab 内存入 cgroup 账本 | 计费 cache 走 KMALLOC_CGROUP 副本 | 容器必开、裸金属按需 |
| `slub_debug=FZ` | boot 参数 | poison/redzone 校验 | 显著 | 调试期 |
| random kmalloc cache | `RANDOM_KMALLOC_CACHES`（6.6 新） | 同尺寸多副本随机路由，粉碎跨 cache 溢出 | 小 | 开 |

**RANDOM_KMALLOC_CACHES 是 v6.6 新特性**：kmalloc-512 变成 512-A/B/C/D 多个物理副本，分配随机路由——攻击者就算溢出一个对象也够不到相邻对象（不同副本物理不相邻）。安全红利几乎免费，值得知道。

## 4. SLAB→SLUB→? 演进时间线

| 年份 | 事件 |
|------|------|
| 1994 | Bonwick 论文（SunOS slab） |
| 2.2/2.4 | Linux slab 合入（原书主角） |
| 2.6.22 | **SLUB 默认**（挑翻 SLAB 的理由：多核扩展+元数据省） |
| 3.x~5.x | per-CPU partial 引入（2.6.x 末期）、freelist random/hardened、slab→folio 化铺垫 |
| 5.9 | `struct slab` 独立类型名；memcg slab objcg 记账重做 |
| 6.4 | SLOB 正式弃用 |
| 6.6 | RANDOM_KMALLOC_CACHES；ptdesc 收尾页表页抽象（见 Ch 3 §3） |

## 5. Buddy → Slab → kmalloc 一图（v6.6 修订版）

```
         应用 / 内核子系统
                │
    ┌───────────┼─────────────┬──────────────┐
    │           │             │              │
 kmem_cache_alloc  kmalloc    alloc_pages    kvmalloc
 (专用类型池)    (档位+泛型)   (整页)        (先kmalloc大档失败→vmalloc)
    │           │             │              │
    └───── SLUB: 快路径 CAS ──┘              │
              per-CPU active/partial         │
              node partial ── list_lock      │
                    │                        │
                 Buddy (Ch 6) ←──────────────┘（高阶失败回退）
```

**kvmalloc 值得记：** 大缓冲的"先连续、后映射"复合策略——先试 kmalloc（物理连续、DMA 友好），失败退 vmalloc（虚拟连续即可）。用户态等价物：先试大页 mmap，失败退小页。

## 6. HFT 精读 checklist（章级收束）

| Slab 概念 | 用户态落地 |
|-----------|-----------|
| 专用 cache | 一种消息/订单一种 pool |
| partial 优先 | 先 pop free list，空了再 grow |
| 快路径无锁 CAS | 每核池 + tagged pointer |
| per-CPU 三件套 | 每核热池→NUMA 冷池→mmap 预留 |
| mempool reserve | 风控/撤单路径预留 slot |
| kmalloc 泛型 | 热路径禁止——固定 size |
| slab 生态开关 | KASAN 调试期才开；FREELIST_HARDENED 生产开 |

## 7. 衔接

- 全章：[§1](./section-1-Slab-分配器的三大核心目标.md) [§2](./section-2-核心数据结构：Cache-与-Slab.md) [§3](./section-3-对象分配与释放.md) [§4](./section-4-尺寸缓存-与-kmalloc-kfree.md) [§5](./section-5-每-CPU-对象缓存.md)
- [Ch 10 页框回收](../../chapter-10-page-frame-reclamation/)：shrinker 的调用方
- [06.5/ch02](../../../06.5-modern-mm/chapter-02-slab-slub-allocator/)：memcg/objcg 记账细节
- [06.7 BPF](../../../06.7-bpf-observability/)：kmem 系列观测工具

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：mempool_alloc 为什么"可以睡眠等归还"？什么情况下这是陷阱？**
A：mempool 假设"借出者终会归还"（bio 完成必还）。若调用方在持有"归还所需资源"的路径上等待——如持块设备队列锁等 bio，而 bio 完成处理也要这把锁——死锁。所以 mempool 用户要求：**借出-归还不是同一临界区内循环依赖**。HFT 池的"等待池非空"同样要审查锁序。

**Q2：shrinker 的 count/scan 两段式为什么必须分开？**
A：direct reclaim 需要先全局问价（count_objects，零 IO）再决定扫多少（scan_objects，可能重活）；干失败/不可回收时还能返回 SHRINK_STOP 收手。一体的回调无法做全局预算。**先报数后执行**是所有资源协商协议的推荐形状。

**Q3：生产内核开 KASAN 的实际代价？**
A：kmalloc 路径 +影子内存读写+redzone 插入，实测网络/调度密集负载掉 15%~30%。HFT 生产路径上等于白送延迟。正确姿势：KFENCE 采样（~0 开销）常开，KASAN 只在压测/复现 bug 的构建上开。

**Q4：`RANDOM_KMALLOC_CACHES` 与 `SLAB_FREELIST_RANDOM` 有何不同？**
A：层次不同。freelist_random 打乱 **同一 slab 内** 的分配顺序（防相邻对象预测）；random_caches 把 **同尺寸请求** 分散到多个物理副本 cache（防跨对象溢出链式受害）。前者 2014 年代，后者 6.6——纵深防御的两层。

**Q5：用户态池怎么对标 `kvmalloc` 的降级策略？**
A：申请大块：先 `madvise(MADV_HUGEPAGE)` 的大页 mmap（TLB 优）；失败/碎片退 4K 小页 mmap。判断依据同内核：**物理连续收益 vs 可用性**。绝不要"一锤子失败就 abort"——降级链是内存子系统面向碎片化的标准容错。

</details>

---
