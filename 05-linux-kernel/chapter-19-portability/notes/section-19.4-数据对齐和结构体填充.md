## ④ 数据对齐和结构体填充

#### 自然对齐 · Natural Alignment

| 规则 | 变量地址必须是 **其类型大小的整数倍** |
|------|--------------------------------------|
| 未对齐访问 | 某些架构 **异常** · 其他架构 **严重变慢** |

#### 结构体填充 · Padding

| 原因 | 编译器在成员间插 **填充字节** 以满足对齐 |
|------|------------------------------------------|
| 优化 | **调整成员顺序** — 把大对齐放前，减空洞 |

**一个具体例子：**

```c
/* 差：24 字节（有 6 字节填充） */
struct bad {
	char  a;    /* 0 */
	            /* 1-3 填充 */
	int   b;    /* 4-7 */
	char  c;    /* 8 */
	            /* 9-15 填充（因为下面要对齐 8） */
	void *d;    /* 16-23 */
};

/* 好：16 字节（无浪费） */
struct good {
	void *d;    /* 0-7   ← 按对齐从大到小排 */
	int   b;    /* 8-11 */
	char  a;    /* 12 */
	char  c;    /* 13 */
	            /* 14-15 尾部填充（结构体整体要 8 字节对齐） */
};
```

| 规则 | 说明 |
|------|------|
| 成员对齐 | 每个成员的地址必须是**其自身大小**的倍数 |
| 结构体整体对齐 | = **最大成员**的对齐要求 |
| 结构体大小 | 必须是整体对齐的**整数倍**（尾部可能补字节） |

> **实践工具：** `pahole`（来自 dwarves 包）能直接打印每个结构体的空洞在哪：
> ```bash
> pahole -C task_struct vmlinux    # 看内核结构体的真实布局
> pahole -C my_struct ./a.out      # 看自己程序的结构体
> ```

| 陷阱 | 结构体 **原样** 发网络/写盘 — **不同架构 padding 不同** → **不兼容** |

```c
/* 协议/on-wire 布局：用 packed 或显式 u8 数组 — 慎用手搓 */
struct hdr {
    __be32 magic;
    __be16 len;
} __packed;
```

> **更稳妥的 on-wire 做法**是**序列化/反序列化函数**（手工按字节偏移读写），
> 而不是给结构体加 `__packed` 然后直接 `memcpy`。原因见下。

---

### `__packed` 的三项真实代价

| 代价 | 说明 |
|------|------|
| **① 非对齐访问** | x86 上硬件自动处理（有性能损失）；ARM 老版本**直接异常**；ARM64 普通 load/store 支持非对齐，但**原子操作不支持** |
| **② 不能做原子操作** | `atomic_t`、`refcount_t`、位操作落在非对齐地址上是未定义的（ARM 上直接 fault） |
| **③ 编译器生成"字节拼接"代码** | 读一个 `u32` 可能变成 4 次 `ldrb` + 移位或运算，**比对齐读慢数倍** |

> 更要命的是：**编译器不知道你的 packed 结构体是非对齐的**。
> 如果你把它传给一个期待 `u32 *` 的函数，编译器会假设指针对齐并可能用 SIMD 指令 → 崩溃。
> ARM64 上这类 bug 表现为偶发 `SIGBUS`。

---

### 缓存行对齐：内核的宏与 HFT 的用法

```c
/* include/linux/cache.h — v6.6 原文 */
#define SMP_CACHE_BYTES L1_CACHE_BYTES              /* :13  通常 64 */
#define ____cacheline_aligned __attribute__((__aligned__(SMP_CACHE_BYTES)))   /* :41 */

#ifdef CONFIG_SMP
#define ____cacheline_aligned_in_smp ____cacheline_aligned   /* :46 */
#else
#define ____cacheline_aligned_in_smp                          /* :48  UP 下为空！ */
#endif
```

| 宏 | 用途 | UP 上行为 |
|----|------|----------|
| `__cacheline_aligned` | 放到 `.data..cacheline_aligned` 段，**总是**对齐 | 仍对齐 |
| `__cacheline_aligned_in_smp` | **只在 SMP 上**对齐 | **UP 上什么都不做**（省内存） |
| `____cacheline_aligned`（四个下划线） | 只加对齐属性，**不改段** | 仍对齐 |

> **UP/SMP 区分的细节很值得学：** 单核机器上不存在"别的核来抢缓存行"这回事，
> 所以对齐纯属浪费内存。内核用条件编译在 UP 上**自动省掉**这些填充——
> 这是"不为不存在的场景付费"的典型体现。

---

### 热冷分离：把最热的字段放进第一缓存行

内核结构体普遍遵守的一个**非书面约定**：**第一缓存行放最热的字段。**

回头看 [Ch 15 里 `vm_area_struct` 的注释](../../chapter-15-process-address-space/notes/section-15.4-内存区域的链表与树.md)：

```c
struct vm_area_struct {
	/* The first cache line has the info for VMA tree walking. */
	unsigned long vm_start;    /* Our start address within vm_mm. */
	unsigned long vm_end;      /* The first byte after our end address ... */
	struct mm_struct *vm_mm;   /* The address space we belong to. */
	...
```

| 为什么这么做 | 说明 |
|-------------|------|
| **缺页/查找路径必定访问前几个字段** | maple tree 遍历就是要 `vm_start`/`vm_end`/`vm_mm` |
| **一次缓存行预取就能拿到** | 如果它们散落在 3 个缓存行里 = 3 次可能的 cache miss |
| 冷字段（统计、调试、链表节点）往后放 | 只有慢路径才访问 |

**HFT 平移到订单簿条目：**

```c
/* 好：一次 cache line 拿全热字段 */
struct order_hot {
	uint64_t price;        /* 8  热：撮合必读 */
	uint32_t qty;          /* 4  热 */
	uint32_t seq;          /* 4  热 */
	uint64_t ts_ns;        /* 8  热 */
	/* 到此 24 字节 */
} __attribute__((aligned(64)));

/* 冷数据单独一张表，按 order_id 索引 */
struct order_cold {
	char     symbol[16];
	uint32_t account;
	uint32_t flags;
	uint64_t parent_id;
	...
};
```

| 拆分的收益 | 说明 |
|-----------|------|
| 一次缓存行能装更多热条目 | 扫描订单簿时 miss 次数下降 |
| 更新冷字段不污染热行 | 改 account 不会把相邻的 price 踢出 L1 |
| 与无锁队列天然契合 | 热结构小 → 拷贝/入队成本低 |

**HFT：** 交易所报文 **固定布局 + ntoh/hton** — 与内核 **`cpu_to_be32`** 同一纪律。

| false sharing 速查 | |
|--------------------|--|
| 症状 | 多线程各自改自己的变量，但**加得越多越慢** |
| 诊断 | `perf c2c`（cache-to-cache），看 HITM 事件落在哪一行 |
| 修法 | `__attribute__((aligned(64)))` 或插入 64 字节填充；**只读的共享数据**反而要**挤在一起**（共享一行更省） |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 结构体对齐规则是什么？如何强制 packed？有什么代价？

<details><summary>答案</summary>

默认对齐：成员按自身大小对齐（int 4 字节对齐），结构体大小 = 最大成员对齐的倍数。`__attribute__((packed))` 取消填充，紧凑排列。代价：1) 非对齐访问在某些架构（ARM）触发异常或变慢（x86 自动处理但有性能损失）；2) 不能用 atomic 操作。HFT 网络协议解析用 packed 结构体映射报文头，性能关键路径用手动偏移。

</details>

**Q2.** 为什么 cache line 对齐对 HFT 至关重要？

<details><summary>答案</summary>

CPU cache line 64 字节。如果两个变量在同一 cache line 且分别被不同 CPU 核频繁写 → false sharing（缓存行 bouncing）。`__attribute__((aligned(64)))` 强制变量独占 cache line。HFT 交易线程和风控线程各自计数器必须 cache line 对齐，否则每秒万次写 → L1 cache 反复 invalidate → 性能下降 10x+。

</details>

**Q3.** 为什么内核结构体普遍把"最热的字段"放在第一缓存行？这个技巧怎么用到订单簿上？

<details><summary>答案</summary>

因为**缓存行是内存访问的最小粒度**（64 字节），一次 miss 会把整行拉进 cache。
如果热路径要访问的字段散落在 3 个缓存行里，就要承受 3 次可能的 miss；
把它们塞进同一行，就只要 1 次。

内核里的现成例子（[Ch 15](../../chapter-15-process-address-space/notes/section-15.4-内存区域的链表与树.md) 里引过）：

```c
struct vm_area_struct {
	/* The first cache line has the info for VMA tree walking. */
	unsigned long vm_start;
	unsigned long vm_end;
	struct mm_struct *vm_mm;
	...
```

缺页处理和 VMA 查找**必定**要读 `vm_start`/`vm_end`——这是最热的字段，所以放在最前面。

**平移到订单簿：**

```c
/* 热：撮合每次都要读，24 字节，一行装得下 */
struct order_hot {
	uint64_t price;   uint32_t qty;
	uint32_t seq;     uint64_t ts_ns;
};

/* 冷：成交回报、日志才用，单独一张表 */
struct order_cold {
	char symbol[16];  uint32_t account;  ...
};
```

| 收益 | 说明 |
|------|------|
| 扫描订单簿时 miss 更少 | 一个 64B 行能装 2 个以上热条目 |
| 改冷字段不污染热行 | 改 account 不会把相邻 price 踢出 L1 |
| 无锁队列拷贝成本更低 | 入队只需拷 24 字节而非整个结构 |

**三条实操要点：**

1. **先测再优化**。用 `perf stat -e cache-misses,cache-references` 看 miss 率，
   或者 `perf c2c` 精确找 false sharing（看 HITM 事件落在哪个地址）。没有数据支撑的"优化"常常是负优化。
2. **只读共享数据反而要挤在一起**。false sharing 只对**多核写**成立——
   如果数据只被一个核写、多核读，那它**共享一行更好**（省缓存容量、省预取带宽）。
   典型如：策略线程写的行情快照，多个下游线程只读 → 打包在一行里。
3. **别把填充用过头**。给 100 万个订单条目各加 64 字节填充 = 多占 40MB 内存，
   L2/L3 装不下反而更慢。热冷分离是**结构性**优化，比无脑加 padding 更有效。

</details>

</details>
---
