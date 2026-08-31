# 02 — AF_XDP 的四个 ring：UMEM 与无锁同步

> **内核文档：** `Documentation/networking/xdp-rings-design.rst`
> **对应 Rosen:** 无（书出版时 AF_XDP 不存在）
> **内核版本:** AF_XDP 4.18+，本文以 **v6.6** 为准，常量取自 `include/uapi/linux/if_xdp.h`

## 文档概述

AF_XDP 的全部机制，本质上就是**四个单生产者/单消费者（SPSC）环形队列**加一块共享内存。
搞懂这四个 ring，AF_XDP 就没有秘密了。

> 📌 **归档说明**：本篇讲的是 **AF_XDP** 的 ring 设计，按主题应属
> [chapter-06-af-xdp](../../chapter-06-af-xdp/)。因来源是 XDP 文档集，
> 保留在此处，并与 chapter-06 建立双向引用。

本篇与兄弟篇的分工：

| 篇 | 讲什么 |
|----|--------|
| [01 XDP 实操](01-xdp-bootlin.md) | 工具链、模式选择、加载失败排查 |
| **02（本篇）** | **AF_XDP 的四个 ring**：UMEM 布局、收发时序、无锁同步、丢包诊断 |
| [03 XDP 架构全景](03-xdp-architecture-lwn.md) | XDP 的五个动作、verifier 约束、与 DPDK 定位 |
| [chapter-06/01](../../chapter-06-af-xdp/notes/01-af-xdp.md) | AF_XDP 的整体使用方式 |
| [chapter-06/03](../../chapter-06-af-xdp/notes/03-af-xdp-umem-layout.md) | UMEM 内存布局细节 |

原笔记 1.3 KB，给了一张"四个 ring"的表格和几行要点。缺的是**时序**（谁先写谁后读）
和**丢包诊断**（ring 相关的丢包怎么定位）。本篇补这两块。

---

## 一、UMEM：用户态与内核共享的那块内存

```
用户态地址空间                          内核（XDP / 驱动）
┌─────────────────────────────┐
│  UMEM（一整块连续内存）        │◄──────── 同一个物理页，双向可见
│                             │
│  ┌─────┬─────┬─────┬─────┐  │
│  │chunk│chunk│chunk│ ... │  │   chunk_size 通常 2048 或 4096
│  │  0  │  1  │  2  │     │  │
│  └─────┴─────┴─────┴─────┘  │
│   ↑                          │
│   headroom（注册时指定）        │
└─────────────────────────────┘
```

注册 UMEM 时告诉内核三件事：

```c
/* include/uapi/linux/if_xdp.h */
struct xdp_umem_reg {
	__u64 addr;         /* 包数据区起始地址 */
	__u64 len;          /* 包数据区长度 */
	__u32 chunk_size;   /* 每个 chunk 的大小 */
	__u32 headroom;     /* 每个 chunk 前面的保留空间 */
	__u32 flags;        /* XDP_UMEM_UNALIGNED_CHUNK_FLAG */
};
```

⚠️ **注意 `headroom` 是注册 UMEM 时逐 chunk 指定的**，不是每个包临时加的。
XDP 要求包前面有 `XDP_PACKET_HEADROOM`（256 字节）的空间，AF_XDP 通过
注册时的 `headroom` 字段一次性满足。

**ring 里传的不是指针，是偏移量（offset）**——因为用户态和内核的地址空间不同，
只有"第几个 chunk / chunk 内偏移"才是双方都能理解的。

### mmap 偏移

四个 ring 都通过 mmap 映射到用户态，各自的页偏移是固定的：

```c
#define XDP_PGOFF_RX_RING			  0
#define XDP_PGOFF_TX_RING		 0x80000000
#define XDP_UMEM_PGOFF_FILL_RING	0x100000000ULL
#define XDP_UMEM_PGOFF_COMPLETION_RING	0x180000000ULL
```

（实际偏移要用 `setsockopt(XDP_MMAP_OFFSETS)` 查询，不要硬编码——
`XDP_MMAP_OFFSETS = 1` 就是干这个的。）

---

## 二、四个 ring

| Ring | 生产者 | 消费者 | 传的是什么 | 方向 |
|------|--------|--------|-----------|------|
| **FILL** | 用户态 | 内核 | 空闲 chunk 的 offset | 用户态 → 内核："这些 buffer 你可以用来收包" |
| **RX** | 内核 | 用户态 | 收到的包的 offset + 长度 | 内核 → 用户态："这些 buffer 里有包了" |
| **TX** | 用户态 | 内核 | 待发送包的 offset + 长度 | 用户态 → 内核："把这些发出去" |
| **COMPLETION** | 内核 | 用户态 | 已发送完成的 chunk offset | 内核 → 用户态："这些 buffer 还给你" |

四个都是 **SPSC（单生产者单消费者）** 无锁队列——这是 AF_XDP 快的关键之一：
**没有锁、没有原子操作的竞争**。

### ⚠️ 关键理解：chunk 的所有权只在 ring 之间流转

```
                    ┌──────────────┐
                    │   空闲池      │
                    └──────┬───────┘
                           │ 用户态把空闲 chunk 放进 FILL
                           v
  用户态 ──FILL──> 内核持有（等着收包）
                           │ 网卡 DMA 写入数据
                           v
  用户态 <──RX─── 内核交还（chunk 里有数据了）
                           │ 用户态处理完
                           v
                    ┌──────────────┐
                    │  放回空闲池    │  ← 或直接再塞进 FILL 复用
                    └──────────────┘
```

**同一个 chunk 在任何时刻只属于一方**。这就是为什么不需要锁，也为什么
**你不能提前把同一个 chunk 塞两次进 FILL**（会造成两个包写到同一块内存）。

---

## 三、收包完整时序

```
【初始化】
  用户态：mmap 四个 ring + UMEM
        把 N 个空闲 chunk 的 offset 批量塞进 FILL ring
        更新 FILL->producer（smp_wmb 之后）

【收包循环】
  ① 内核（驱动/XDP）：网卡 DMA 把包写进某个 chunk
       ↓
  ② 内核：从 FILL ring 取出一个 chunk offset
       - FILL 空了 → rx_fill_ring_empty_descs++，包被丢弃 ★
       ↓
  ③ 内核：把「offset + 包长度」写进 RX ring
       更新 RX->producer（smp_wmb 保证数据先落地、指针后更新）
       ↓
  ④ 用户态：读 RX->producer（smp_rmb），发现有新条目
       从 RX ring 取出 offset + 长度
       直接读 UMEM[offset] —— 零拷贝，包就在这里
       ↓
  ⑤ 用户态：处理完，更新 RX->consumer
       ↓
  ⑥ 用户态：把这个 chunk 重新塞进 FILL ring（或攒一批再塞）
       更新 FILL->producer
       ↓
     回到 ①
```

### 内存序：为什么必须有 `smp_wmb()` / `smp_rmb()`

生产者写 ring 条目的顺序是：**先写数据，再更新 producer 指针**。
如果没有写屏障，CPU 或编译器可能把这两步重排——消费者看到 producer 更新了，
但读到的数据还是旧的。

```c
/* 生产者 */
ring->entries[idx] = data;      /* ① 写数据 */
smp_wmb();                      /* ② 屏障：保证 ① 对消费者可见 */
WRITE_ONCE(ring->producer, idx+1);  /* ③ 更新指针 */

/* 消费者 */
idx = READ_ONCE(ring->producer);    /* ① 读指针 */
smp_rmb();                      /* ② 屏障：保证看到 ③ 时也能看到 ① */
data = ring->entries[idx];      /* ③ 读数据 */
```

**对 HFT 的含义**：这两条屏障是**每包必经**的开销。它们不是锁（x86 上 `smp_wmb()`
基本是编译器屏障，不产生指令），但**在弱内存序架构上（ARM）是真指令**。
这意味着——**在 ARM 平台（如树莓派 5、AWS Graviton）上，AF_XDP 的 per-packet
同步开销比 x86 更高**。这是选型时值得考虑的一点，也是 eBPF 学习从 x86 迁到
ARM 时最容易忽略的差异。

---

## 四、ring 大小与丢包

ring 大小由应用通过 setsockopt 指定（`XDP_RX_RING` / `XDP_TX_RING` /
`XDP_UMEM_FILL_RING` / `XDP_UMEM_COMPLETION_RING`），**不是内核常量**。

**核心原则**：

| Ring | 该多大 | 太小的后果 |
|------|--------|-----------|
| **FILL** | ≥ RX ring 大小 + 在用户态处理中的包数 | **★ 最主要的丢包原因**：`rx_fill_ring_empty_descs` |
| **RX** | 能吸收突发（行情开盘那一瞬） | `rx_ring_full` |
| **TX** | ≥ 单次突发发送量 | `tx_ring_empty_descs`（发送侧） |
| **COMPLETION** | ≥ TX ring 大小 | 已完成 buffer 回收不及时 |

### ⚠️ FILL ring 是 AF_XDP 最常见的丢包点

原因很直白：**内核要收包时，FILL ring 里必须有空闲 chunk**。
如果你的用户态程序处理慢了（或批处理粒度太大，攒太多才回塞 FILL），
内核就会因为"没有 buffer 可用"而丢包。

**这是 AF_XDP 与 DPDK 最不一样的地方**：
- DPDK 的 mbuf 池由 PMD 自己管，收包时如果池空了，包就留在网卡 ring 里等下一轮
- AF_XDP 的 FILL ring 空了，包**直接被丢弃**（`rx_fill_ring_empty_descs++`）

所以 AF_XDP 调优的第一条是：**FILL ring 要足够大，且回塞要及时**。
常见做法是攒一批（如 64 个）再回塞，平衡系统调用/更新开销与丢包风险——
这个批大小直接决定了"最多能容忍多少个包的处理延迟"。

---

## 五、`NEED_WAKEUP` 与忙轮询

```c
#define XDP_USE_NEED_WAKEUP (1 << 3)
#define XDP_RING_NEED_WAKEUP (1 << 0)
```

默认情况下，AF_XDP 的收包靠**中断唤醒**——每来一个包（或每批）触发一次软中断，
把包交给用户态。这对延迟不友好。

开启 `XDP_USE_NEED_WAKEUP` 后语义变成：

- 内核**不再自动唤醒**用户态进程
- 如果内核需要用户态介入（比如 FILL ring 需要补充、TX ring 需要 kick），
  会在 ring 的 flags 里置 `XDP_RING_NEED_WAKEUP`
- **用户态自己检测这个标志并决定何时 kick**——通常配合**忙轮询**

```c
/* 伪代码 */
if (xsk_ring_prod__needs_wakeup(&fill_ring))
    poll(fds, ...);        /* 或者 sendto() 触发一次 kick */
```

**对 HFT**：开启 `XDP_USE_NEED_WAKEUP` + 用户态忙轮询，是 AF_XDP 达到
接近 DPDK 延迟的**必要条件**。不开的话，你的延迟里会叠加中断唤醒的抖动。

⚠️ 代价是**独占一个 CPU 核**。这和 DPDK 的 busy poll 一样，是"用核换延迟"。

---

## 六、零拷贝 vs copy 模式：怎么确认你在哪种

```c
#define XDP_SHARED_UMEM	(1 << 0)
#define XDP_COPY	(1 << 1)      /* Force copy-mode */
#define XDP_ZEROCOPY	(1 << 2)      /* Force zero-copy mode */
#define XDP_USE_NEED_WAKEUP (1 << 3)
#define XDP_USE_SG	(1 << 4)      /* multi-buffer（大于一个 chunk 的包） */
```

**不指定时内核自动选**：驱动支持就零拷贝，不支持就静默降级到 copy 模式。
⚠️ 和 XDP 的 native/generic 降级一样，**这个静默降级会毁掉你的性能结论**。

查询方法（v6.6 有专门的接口）：

```c
/* include/uapi/linux/if_xdp.h */
#define XDP_OPTIONS		8
#define XDP_OPTIONS_ZEROCOPY (1 << 0)

struct xdp_options {
	__u32 flags;
};
```

```bash
# 命令行看（xdp-tools）
xdp-loader status          # 会显示 AF_XDP socket 的模式
```

**实践建议：显式指定 `XDP_ZEROCOPY`**。这样如果不支持，你会**直接得到错误**，
而不是静默地跑在一个慢得多的模式上还以为很快。

### copy 模式慢在哪

copy 模式下，内核把包**复制**进 UMEM 的 chunk，而不是让网卡直接 DMA 到那里。
于是：
- 每包多一次内存拷贝（按包长，几百到几千字节）
- 更重要的是：失去了"NIC DMA 直达用户态内存"这个核心优势

**对 HFT 的结论**：**copy 模式的 AF_XDP 基本没有意义**——它相比普通
`AF_PACKET`/`recvmmsg` 的收益有限，却要你重写整个收包逻辑。
要么上零拷贝，要么干脆用内核协议栈。

---

## 七、观测：`xdp_statistics` 的六个计数器（诊断核心）

```c
/* include/uapi/linux/if_xdp.h */
struct xdp_statistics {
	__u64 rx_dropped;                  /* 其他原因丢弃 */
	__u64 rx_invalid_descs;            /* 描述符无效 */
	__u64 tx_invalid_descs;            /* 描述符无效 */
	__u64 rx_ring_full;                /* RX ring 满 */
	__u64 rx_fill_ring_empty_descs;    /* FILL ring 空，取不到 buffer ★ */
	__u64 tx_ring_empty_descs;         /* TX ring 空 */
};
```

通过 `setsockopt(XDP_STATISTICS)` 读取。**这六个数字就是 AF_XDP 的体检表**：

| 计数器 | 含义 | 处置 |
|--------|------|------|
| **`rx_fill_ring_empty_descs`** | **FILL ring 空，内核没 buffer 可用** ★ | **最主要的问题**：增大 FILL ring、减小回塞批次、加快用户态处理 |
| `rx_ring_full` | RX ring 满，用户态取不过来 | 增大 RX ring、加快消费 |
| `rx_dropped` | 其他原因（如 XDP 程序 DROP、驱动丢） | 看 XDP 程序逻辑与 `ethtool -S` |
| `tx_ring_empty_descs` | TX ring 空（发送侧） | 通常无害，只是没包可发 |
| `rx_invalid_descs` / `tx_invalid_descs` | 描述符非法 | **程序 bug**：offset 越界、重复提交同一 chunk |

**最后一行要特别注意**：`invalid_descs` 增长几乎一定是**你的代码有 bug**，
最常见的是**把同一个 chunk 提交了两次**（所有权被破坏，见第二节）。

```bash
# xdp-tools 的 xdpsock 会直接打印这些
xdpsock -i eth0 -r -N

# 自己读：setsockopt(XDP_STATISTICS)
```

⚠️ 注意 `XDP_PKT_HEADROOM`：如果你自己写 AF_XDP 程序，访问包数据时要跳过
注册 UMEM 时指定的 `headroom`，否则读到的位置不对。

---

## HFT 要点

- **四个 ring 都是 SPSC 无锁队列**——这是 AF_XDP 快的根本，但也意味着**单 socket 不能多线程并发收包**（要做多队列 + 多 socket）。
- **`rx_fill_ring_empty_descs` 是第一诊断指标**：它涨说明你的用户态补充 buffer 跟不上，这是 AF_XDP 最主要的丢包来源。
- **FILL ring 空了包就直接丢**，不像 DPDK 会留在网卡 ring 里等下一轮。这是 AF_XDP 与 DPDK 的关键行为差异。
- **必须显式指定 `XDP_ZEROCOPY`**：不指定会静默降级到 copy 模式，毁掉你的性能结论。**copy 模式的 AF_XDP 对 HFT 基本没意义**。
- **`XDP_USE_NEED_WAKEUP` + 忙轮询是低延迟的必要条件**，代价是独占一个核。
- **ARM 上的每包同步开销比 x86 高**：`smp_wmb()`/`smp_rmb()` 在弱内存序架构上是真指令。选型和跨平台移植时要考虑。
- **`invalid_descs` 增长 = 你的代码有 bug**，最可能是把同一个 chunk 提交了两次。
- **ring 里传的是 offset 不是指针**——用户态和内核地址空间不同，只有 chunk 偏移是双方都能理解的。

## 与 Rosen 3.x 的差异

Rosen 写作时 AF_XDP 不存在。但从"内核-用户态数据传递"的角度有一条延续：

| Rosen 时代的做法 | AF_XDP |
|-----------------|--------|
| `copy_to_user()` 把包从内核拷到用户态 | **零拷贝：NIC 直接 DMA 到用户态 UMEM** |
| 每包一次系统调用 + 两次特权级切换 | 批量操作 ring，**可以完全不进内核**（忙轮询时） |
| socket 接收队列由内核管理 | **FILL ring 由用户态管理**（责任转移） |
| 内核决定丢弃时机 | **用户态负责补 buffer**，补不上就丢 |

**最重要的观念转变**：AF_XDP 把"缓冲区管理"的责任从内核**转移到了用户态**。
这带来了零拷贝，也带来了新的失效模式——**你的程序慢了，内核就得丢包**。
DPDK 也是这个模型，但 DPDK 池空时包留在网卡里；AF_XDP 是直接丢。

---

## 代码自测

<details>
<summary>Q1：AF_XDP 收包在高负载下丢包严重，<code>xdp_statistics</code> 里 <code>rx_fill_ring_empty_descs</code> 一直涨，但 <code>rx_ring_full</code> 是 0。怎么调？</summary>

<b>答：</b>这个组合的含义很明确：<b>不是用户态取包慢，是内核没 buffer 可用</b>。

- `rx_ring_full = 0` → RX ring 没满，用户态消费得挺快
- `rx_fill_ring_empty_descs` 涨 → 内核想收包时，FILL ring 里<b>没有空闲 chunk</b>

问题出在<b>回塞 FILL 的节奏</b>上。常见原因：

1. **批处理粒度太大**。你可能攒了 256 个包处理完才回塞一次 FILL。
   这期间内核最多只能用 FILL ring 里剩余的 chunk。把批次调小（如 64）。
2. **FILL ring 太小**。至少要能覆盖"RX ring 深度 + 用户态在处理的包数"。
   经验：FILL ring ≥ 2 × RX ring。
3. **处理逻辑里有阻塞**。某个包触发了慢路径（日志、锁、系统调用），
   整批 chunk 的回塞都被推迟。

<b>调完怎么验证</b>：盯着 `rx_fill_ring_empty_descs`，目标是<b>归零</b>。
注意丢包往往是突发性的（行情开盘瞬间），所以要看峰值而不是平均值。

<b>为什么这和 DPDK 不同</b>：DPDK 的 mempool 空了，包会留在网卡的 Rx ring 里
等下一轮 `rx_burst`，表现为延迟升高而非丢包。AF_XDP 是<b>直接丢</b>——
FILL ring 空 = 内核没有可用的 DMA 目标 = 包被丢弃。这个行为差异在容量规划时要算进去。
</details>

<details>
<summary>Q2：你没指定 <code>XDP_ZEROCOPY</code> 也没指定 <code>XDP_COPY</code>，程序跑起来一切正常。这有什么问题？</summary>

<b>答：</b>问题在于<b>你不知道自己跑在哪个模式</b>——不指定时内核自动选择，
驱动不支持就<b>静默降级到 copy 模式</b>。

这和 XDP 的 native/generic 降级是同一类坑：功能完全正常，性能差一个量级，
而你会基于错误的模式得出结论（"AF_XDP 也不过如此"）。

copy 模式下，内核要把每个包<b>复制</b>进 UMEM 的 chunk，而不是让网卡 DMA 直接写进去。
于是：
- 每包多一次内存拷贝（几百到几千字节，按包长）
- 失去"NIC DMA 直达用户态内存"这个 AF_XDP 的核心优势

<b>正确做法：显式指定</b>

```c
int opt = XDP_ZEROCOPY;
setsockopt(xsk_fd, SOL_XDP, XDP_ZEROCOPY, &opt, sizeof(opt));
```

或者用 `XDP_OPTIONS` 查询后确认：

```c
struct xdp_options opts = {};
socklen_t len = sizeof(opts);
getsockopt(xsk_fd, SOL_XDP, XDP_OPTIONS, &opts, &len);
if (opts.flags & XDP_OPTIONS_ZEROCOPY)
    /* 确实是零拷贝 */;
```

显式指定的好处是<b>如果不支持就直接报错</b>，而不是静默地慢。

<b>对 HFT 的补充结论</b>：copy 模式的 AF_XDP 基本没有意义——相比
`recvmmsg` + 内核协议栈收益有限，却要你重写整个收包逻辑。
<b>要么上零拷贝，要么干脆用内核协议栈。</b>
</details>

<details>
<summary>Q3：你把 AF_XDP 程序从 x86 服务器移植到 ARM（如树莓派 5 / Graviton），PPS 掉了不少，但 CPU 占用看起来差不多。可能是什么原因？</summary>

<b>答：</b>先怀疑 <b>ring 同步的内存屏障开销</b>。

AF_XDP 的四个 ring 靠 `smp_wmb()` / `smp_rmb()` 保证生产者-消费者的可见性顺序：

```c
/* 生产者 */
ring->entries[idx] = data;
smp_wmb();                          /* ← 这里 */
WRITE_ONCE(ring->producer, idx+1);
```

在 **x86（TSO，强内存序）** 上，`smp_wmb()` 基本只是<b>编译器屏障</b>，
不产生实际的 CPU 指令——store-store 本来就不会重排。

在 **ARM（弱内存序）** 上，`smp_wmb()` 会展开成真正的屏障指令（如 `dmb ishst`），
<b>每包都要执行两次</b>（生产者写、消费者读各一处）。

所以：
- 每包多几条屏障指令 → PPS 上限下降
- 但这是"执行了更多指令"而非"占用更多时间片" → CPU 占用率看起来差不多

<b>验证</b>：
```bash
# 对比两边的每包耗时（用 bpftool 或自己打时间戳）
# 如果 ARM 上每包多出几十 ns，且和屏障指令数吻合，就坐实了
```

<b>处置</b>：
- 增大批处理粒度（一次处理更多包，摊薄每包的屏障开销）
- 检查内核是否用了最优的屏障实现（ARM64 上 `dmb ishst` vs `dmb ish` 差别不小）
- 选型时就把这一点算进去——ARM 上的 AF_XDP 绝对性能确实不如 x86

<b>延伸</b>：这个差异不只影响 AF_XDP，也影响所有用 lockless ring 的机制
（io_uring、DPDK 的 rte_ring 同理）。在 ARM 上做低延迟，
要把"每包的原子/屏障操作"当作一项显式成本来算。
</details>

---

→ 前一篇：[01 XDP 实操](01-xdp-bootlin.md)
→ 后一篇：[03 XDP 架构全景](03-xdp-architecture-lwn.md)
→ 相关：[chapter-06 AF_XDP](../../chapter-06-af-xdp/) · [chapter-07 XDP vs DPDK](../../chapter-07-xdp-redirect-dpdk/)
