# Ch 8 §1 Slab 分配器的三大核心目标（及 SLUB 时代的变化）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`mm/slub.c` 头部设计注释、`mm/slab_common.c`）

---

## 本节讲什么

原书给 slab 立了三大设计目标。二十年后 SLUB 继承了哪些、推翻了哪些？先记住原书目标，再对照 v6.6 现实——这决定了"从 slab 学到的东西哪些能搬到用户态对象池设计"。

---

## 1. 原书三大目标

| 目标 | 2.4 slab 机制 | HFT 用户态镜像 |
|------|---------------|----------------|
| **消除内部碎片** | 小于一页的请求按对象大小从 slab 切分，而非整页浪费 | object pool 只分配 `sizeof(Order)` 对齐块 |
| **缓存常用对象** | 释放对象保持初始化状态挂 cache 上，复用免 ctor/dtor | free list 复用 Order，reset 字段而非 new/delete |
| **优化硬件缓存（着色）** | slab 剩余空间作 color 偏移，同类型对象错开 cache line | `alignas(64)`、按 core 分 arena、padding 热字段 |

## 2. SLUB（v6.6 默认）对三目标的取舍

**SLUB = Unqueued Slab Allocator**（Christoph Lameter，2007 合入）。设计哲学大转弯：从"管理精细"转向"路径极简"。

| 原书目标 | SLUB 处置 | v6.6 证据 |
|----------|-----------|-----------|
| 消除内部碎片 | **保留**（基本盘） | 尺寸档位+专用 cache 不变 |
| 对象缓存/ctor | **弱化**——ctor/dtor 基本退役，复用即"拿一块干净的内存" | `kmem_cache_create` 的 ctor 参数仅 debug 用途保留 |
| 着色 | **删除**——被 `CONFIG_SLAB_FREELIST_RANDOM` 取代：freelist 顺序随机化，对象物理布局不可预测（防 heap 溢出利用）+ 天然错开冲突 | slub.c 头注释；`slab_freelist_random` 初始化 |

**SLUB 新立的三个目标（原书没有）：**

1. **快路径无锁**——`this_cpu_cmpxchg_double(freelist, tid)` 一条原子指令完成分配（slub.c:421 注释），无 cache 级自旋锁
2. **元数据最小**——空闲链 **嵌在对象体内**（object 前一个 word 当 next 指针），不设 bufctl 数组、不设 on/off-slab 描述符
3. **多核可扩展**——per-CPU active slab + per-CPU partial 链，避开 `node->list_lock` 中央锁（"avoid taking it as much as possible"，slub.c 头注释原文）

**着色之死值得一句纪念：** 着色解决的是"不同 slab 起始地址 mod 64 相同 → 同序号对象挤同一 line"。SLUB 的答案更粗暴——freelist 随机化后访问顺序本身无序，冲突概率自然摊薄，还附赠安全收益。**工程上的通用教训：随机化常常是比精确布局更便宜的同效手段。**

## 3. 三分配器血统表

| | SLAB（原书主角） | SLUB（v6.6 默认） | SLOB |
|---|------------------|-------------------|------|
| 时代 | 1994 Bonwick / Linux 2.2 | 2007 Lameter | 嵌入式小内存 |
| per-slab 元数据 | 完整（bufctl 数组、三链表） | 极简（嵌入 freelist） | 压栈式 |
| 三链表 full/partial/free | ✓ 每 cache | ✗ 只保留 per-node partial | ✗ |
| 锁 | cache 级自旋 | cmpxchg 快路径 + local lock | 全局锁（小内存可忍） |
| v6.6 状态 | 仍可选（`CONFIG_SLAB`），维护半弃 | **默认** | `CONFIG_SLOB`（6.4 起正式弃用告警） |

## 4. HFT / 嵌入式关联

| 概念 | 用户态落地 |
|------|-----------|
| 专用 cache（一类型一池） | 一种消息/订单一种 pool |
| 复用免重初始化 | pool 分配一次初始化，复用只 reset 热字段 |
| 着色→随机化的演化 | 用户态池无需着色；学 SLUB 用 freelist 而非 bump allocator，天然错行 |
| 快路径无锁 | 每核本地池 + 一次 CAS 换主（见 §3/§5） |
| kmalloc 泛型档位 | 热路径禁止——固定 size pool（§4） |

## 5. 衔接

- [§2 核心数据结构](./section-2-核心数据结构：Cache-与-Slab.md)：SLUB 的三个 struct
- [05-linux-kernel](../../../05-linux-kernel/)：LKD 的 slab 叙述同样偏旧，互为印证
- [06.5/ch02 slub](../../../06.5-modern-mm/chapter-02-slab-slub-allocator/)：现代视角完整版

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：SLUB 为什么敢删掉 ctor/dtor？**
A：实测绝大多数 cache 的 ctor 只是 memset/链表初始化，而对象复用时调用者本来就要重新初始化业务字段。SLUB 把"池只保证干净内存、初始化归调用者"定为契约，快路径少一层间接调用。教训：**池的职责边界越窄，路径越快**。

**Q2：着色和 freelist 随机化，哪个对性能更好？**
A：着色是确定性优化（防系统性冲突），随机化是概率性摊薄。对 HFT 用户态池：确定性布局（槽位错开 64B 边界）**尾延迟更可控**——随机化把 p50 和 p99.9 拉近，但引入方差。内核选随机化主因是安全收益，性能上两者差异很小。

**Q3：`CONFIG_SLAB_FREELIST_RANDOM` 防的是什么攻击？**
A：heap 溢出类利用需要预测"下一个分配的对象在哪"。确定性 freelist = 攻击者可计算 victim 位置。随机化后即使溢出改写 freelist 指针也难定向。配套的 `CONFIG_SLAB_FREELIST_HARDENED` 再给 freelist 指针加 XOR 加密。

**Q4：SLOB 为什么被弃用？**
A：小内存设备现在也够 RAM 跑 SLUB；SLOB 不支持 memcg/debugfs/sysfs 全家桶，维护成本 > 收益。6.4 起 Kconfig 明确 deprecated。嵌入式支线遇到老讲义提 SLOB，按"历史方案"读。

**Q5：用户态对象池要学 SLUB 的哪一条最重要？**
A：**空闲链嵌对象体**（free slot 自带 next 指针）——池的元数据成本 = 0 额外内存，cache 密度最高。对应实现：拿回 slot 时先读首 word 存的 next，再往 slot 写业务数据。jemalloc/tcmalloc 的 thread cache 同构。

</details>

---
