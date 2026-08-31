# 02 — 驱动与内核的契约（txrx.rst 视角）

> **内核文档：** `Documentation/networking/txrx.rst`
> **对应 Rosen:** Ch1（收发包路径）
> **内核版本:** 以 v6.6 为准，行号已核对源码

## 文档概述

上一篇 [01-tx-path-bootlin](01-tx-path-bootlin.md) 讲的是**协议栈怎么把包交给网卡**。本篇换一个视角：站在**驱动作者**的位置，看内核要求驱动遵守哪些约定。

为什么要看这个？因为**你在 HFT 里遇到的很多"网络延迟"，其实是驱动对契约的某种实现方式造成的**——中断合并策略、Tx ring 清理时机、DMA 映射方式、skb 释放路径，全是驱动自己选的。看不懂契约，就只能对着 `ethtool -S` 的计数器瞎猜。

本篇与兄弟篇的分工：

| 篇 | 视角 | 关键词 |
|----|------|--------|
| [01](01-tx-path-bootlin.md) | 协议栈俯视 | qdisc、tc egress、延迟构成 |
| **02（本篇）** | **驱动作者** | `ndo_start_xmit`、DMA、ring 清理、释放路径 |
| [03](03-sock-sk-buff.md) | 数据结构 | `sk_buff` 分配/克隆/释放 |
| [04](04-sk-buff-xdp-buff.md) | 数据表示 | `xdp_buff` ↔ `sk_buff` 转换 |

> 💡 本篇的"现代驱动关键点"表格原笔记只列了 5 行对比（传统 vs 现代）。下面把它展开成**契约条目**，每一条都说明"不遵守会怎样"。

---

## 一、RX 契约：驱动收包时该做什么

```
NIC 收到帧 → DMA 写入 Rx ring 的 buffer（page_pool 分配）
NIC 写回 Rx 描述符 → 触发中断（受 rx-usecs / rx-frames 合并）
驱动中断 handler → napi_schedule(&napi)
驱动的 napi_poll(napi, budget) 被调用：
  ├─ 从 Rx ring 取包（最多 budget 个）
  ├─ 为每个包跑 XDP 程序（native XDP，如有）
  │   ├─ XDP_DROP     → page 立刻回收给 page_pool
  │   ├─ XDP_PASS     → 构造 sk_buff 继续走协议栈
  │   ├─ XDP_TX       → 原路反弹
  │   └─ XDP_REDIRECT → 交给 AF_XDP / CPUMAP / DEVMAP / veth
  ├─ 无 XDP → 直接构造 sk_buff
  ├─ napi_gro_receive()  ← 注意：不是直接 netif_receive_skb()
  ├─ 补充 Rx ring 的新 buffer（refill）
  └─ return work_done
     ├─ work_done < budget → napi_complete_done()，重新开中断
     └─ work_done == budget → 直接返回，softirq 会让它再跑一轮
```

### 契约条目

| # | 约定 | 违反后果 |
|---|------|---------|
| 1 | XDP 必须**先于** `sk_buff` 构造 | 否则 XDP_DROP 省不掉 skb 分配开销，XDP 失去意义 |
| 2 | 必须用 `napi_gro_receive()` 而非 `netif_receive_skb()` | GRO 收不到包，吞吐暴跌 |
| 3 | 严格遵守 `budget` 上限 | 超了会导致单轮 softirq 过长，其他 queue 饿死、调度延迟上升 |
| 4 | `work_done == budget` 时不调用 `napi_complete_done()` | 重入时丢包；反过来漏调会导致中断关死后不再收包 |
| 5 | 页面从 `page_pool` 分配 | 不用 page_pool 的话 XDP 无法工作（XDP 依赖 page_pool 的回收语义） |
| 6 | 中断合并通过 `ethtool -C` 暴露 | 无法调优，收包延迟下不来 |

**第 3 条对 HFT 的直接含义**：`budget` 越大，单轮 softirq 越长，**别的 CPU 核上的收包线程被推迟的时间就越长**。低延迟场景要的是**小 budget + 频繁轮询**，不是"一次多收点"。

---

## 二、TX 契约：`ndo_start_xmit` 的返回值

这是本篇最有价值的一节，因为**常见资料（包括 Rosen）里写的返回值已经过时了**。

### v6.6 的真实枚举

```c
/* include/linux/netdevice.h:124 */
enum netdev_tx {
	__NETDEV_TX_MIN	 = INT_MIN,	/* make sure enum is signed */
	NETDEV_TX_OK	 = 0x00,	/* driver took care of packet */
	NETDEV_TX_BUSY	 = 0x10,	/* driver tx path was busy*/
};
```

**只有两个值。** 老资料里的 `NETDEV_TX_LOCKED`（"tx 锁已被别的 CPU 拿着"）在 v6.6 已经**不存在**——txq 锁的语义改了，驱动不再需要返回这个值。

### 返回值怎么被处理

```c
/* include/linux/netdevice.h */
static inline bool dev_xmit_complete(int rc)
{
	/*
	 * Positive cases with an skb consumed by a driver:
	 * - successful transmission (rc == NETDEV_TX_OK)
	 * - error while transmitting (rc < 0)
	 * - error while queueing to a different device (rc & NET_XMIT_MASK)
	 */
	if (likely(rc < NET_XMIT_MASK))
		return true;
	return false;
}
```

配套常量（`include/linux/netdevice.h:114`）：

```c
#define NET_XMIT_SUCCESS	0x00
#define NET_XMIT_DROP		0x01	/* skb dropped */
#define NET_XMIT_CN		0x02	/* congestion notification */
#define NET_XMIT_MASK		0x0f	/* qdisc flags */
```

所以判定逻辑是 `rc < 0x0f`。代入：

| 返回值 | 值 | `dev_xmit_complete` | 内核动作 |
|--------|-----|---------------------|---------|
| `NETDEV_TX_OK` | 0x00 | true | skb 已被驱动接管，内核**不再碰它** |
| 负数（驱动出错） | < 0 | true | 同样视为已消费（错误已统计） |
| `NET_XMIT_CN` | 0x02 | true | 拥塞通知，但 skb 已消费 |
| **`NETDEV_TX_BUSY`** | **0x10** | **false** | **驱动没接管 skb → 内核 requeue 重发** |

### ⚠️ `NETDEV_TX_BUSY` 是"设计缺陷的兜底"，不是正常机制

重点来了：**一个正确的驱动不应该频繁返回 `NETDEV_TX_BUSY`**。

正规的流控是靠队列的 stop / wake：

```c
/* 驱动侧：Tx ring 快满时 */
netif_tx_stop_queue(txq);        /* 告诉上层：别再给我了 */
...
/* 完成中断里清理完 ring 后 */
netif_tx_wake_queue(txq);        /* 恢复，并触发 qdisc 出队 */
```

`netif_tx_stop_queue()` 之后，qdisc 层看到队列 stop 就不会再 dequeue，`ndo_start_xmit` 根本不会被调用。这才是**零成本**的流控。

而 `NETDEV_TX_BUSY` 是"驱动发现真的发不出去了"时的最后手段：内核要把 skb 重新入队、`qdisc->qstats.requeues++`、然后**重新调度**。这整个过程是**纯延迟**，而且会让 `tc -s qdisc` 里的 `requeues` 计数上涨。

**观测结论**：

```bash
tc -s qdisc show dev eth0
# 如果 requeues 持续增长 → 你的 Tx ring 太小 / 队列 stop 阈值设置不合理
# 这是纯浪费，不是"正常的背压"
```

对 HFT，requeues 增多意味着发包延迟出现**无法预测的尖峰**——因为它发生在队列将满未满的边界上，属于典型的双峰分布第二个峰的来源。

---

## 三、DMA：映射与解映射的时点

```
ndo_start_xmit(skb)
  ├─ dma_map_single(...) / dma_map_page(...)     ← 流式映射，CPU 之后不能再碰这块内存
  ├─ 填 Tx ring 描述符
  └─ 写门铃（除非 xmit_more）

        ───── 网卡 DMA 读走数据并发送 ─────

TX 完成中断 / NAPI poll
  ├─ 读回描述符的完成位
  ├─ dma_unmap_single(...) / dma_unmap_page(...)
  └─ 释放 skb
```

**契约：在网卡确认发送完成之前，不能 unmap、不能释放、不能复用。**

这一点决定了 `MSG_ZEROCOPY` 的真实代价（详见 [01 篇](01-tx-path-bootlin.md)）：你的用户态 buffer 被 pin 住挂在 `skb_shinfo->frags[]` 上，只有当**完成中断**处理完、`SKBTX_ZEROCOPY_FRAG` 的通知进了 error queue，那块内存才重新归你。

完成中断什么时候来？取决于中断合并：

```bash
ethtool -c eth0 | grep -i tx
#   tx-usecs / tx-frames 非 0 → 完成通知被攒着，buffer 释放被推迟
```

所以 `ethtool -C eth0 tx-usecs 0` 不只是"降低延迟"，它还**直接把你可用的发送缓冲池变大**。

---

## 四、skb 释放路径：四个函数，差别很大

驱动清理 Tx ring 时要释放 skb，用哪个函数**不是随意的**——它们在**统计语义**和**释放速度**上都不同。

| 函数 | 场景 | 计入丢包吗 | 走 per-CPU 缓存吗 |
|------|------|-----------|------------------|
| `kfree_skb(skb)` | 通用释放 | **计入**（`SKB_DROP_REASON_NOT_SPECIFIED`） | 否 |
| `__kfree_skb(skb)` | 内部实现 | 计入 | 否 |
| `consume_skb(skb)` | 正常投递完成 | **不计入**（`SKB_CONSUMED`） | 否 |
| `napi_consume_skb(skb, budget)` | **NAPI 上下文**（推荐） | 不计入 | **是** ← 快 |

### `napi_consume_skb` 的实际实现（v6.6 `net/core/skbuff.c`）

```c
void napi_consume_skb(struct sk_buff *skb, int budget)
{
	/* Zero budget indicate non-NAPI context called us, like netpoll */
	if (unlikely(!budget)) {
		dev_consume_skb_any(skb);
		return;
	}

	DEBUG_NET_WARN_ON_ONCE(!in_softirq());

	if (!skb_unref(skb))
		return;

	/* if reaching here SKB is ready to free */
	trace_consume_skb(skb, __builtin_return_address(0));

	/* if SKB is a clone, don't handle this case */
	if (skb->fclone != SKB_FCLONE_UNAVAILABLE) {
		__kfree_skb(skb);
		return;
	}

	skb_release_all(skb, SKB_CONSUMED, !!budget);
	napi_skb_cache_put(skb);      /* ← per-CPU 缓存，避免 slab 分配器往返 */
}
```

三个关键细节：

1. **`budget` 参数不是"预算"，是"你是否在 NAPI 上下文"的标记**。`budget == 0` 表示非 NAPI 调用（如 netpoll），退化成 `dev_consume_skb_any()`。这个参数名极具误导性。
2. **最后一个参数是 `!!budget`，控制是否走 `napi_skb_cache_put()`**——即是否把 skb 头放进 per-CPU 缓存。这是"NAPI 上下文释放更快"的真正来源：省了一次 slab 分配器往返。
3. **clone 的 skb 走不到缓存**（`skb->fclone != SKB_FCLONE_UNAVAILABLE` 就退化成 `__kfree_skb`）。所以**克隆过的包释放更贵**。

### ⚠️ 统计陷阱：用错函数会让你的监控说谎

`kfree_skb` 与 `consume_skb` 的区别是**语义**上的：

- `kfree_skb()` → `SKB_DROP_REASON_NOT_SPECIFIED` → **会被丢包监控（drop_monitor、kfree_skb_reason tracepoint）记录下来**
- `consume_skb()` / `napi_consume_skb()` → `SKB_CONSUMED` → **正常投递，不计丢包**

所以：如果你在驱动的完成路径里误用了 `kfree_skb()`，那么**每个成功发出的包都会被统计成一次丢包**。`dropwatch` 里会看到天量的 `skb_kfree` 事件，而实际上一个包都没丢。这是驱动开发里很经典的坑，反过来也说明——**看到 `kfree_skb` 事件猛增，先怀疑是不是有人在错误的地方用了错误的释放函数**，而不是立刻认定在丢包。

---

## 五、page_pool：为什么现代驱动不再 `alloc_page`

| | 传统（`alloc_page`） | 现代（`page_pool`） |
|---|---|---|
| 分配成本 | 每次走伙伴系统 | per-CPU 缓存，命中即返回 |
| 回收 | `put_page()` 回伙伴系统 | 驱动/XDP 直接回收进池 |
| XDP 支持 | 无（XDP 依赖 page_pool） | 原生支持 |
| NUMA | 需要自己管 | 池按 NUMA 节点创建 |
| DMA 映射 | 每次 map/unmap | **可保持映射**（`PAGE_POOL_DMA_MAP`），省掉每次 map |

最后一行是关键：page_pool 可以**预先做好 DMA 映射并保持住**，收发循环里就省掉了 `dma_map`/`dma_unmap` 这一对（每次约几十到几百 ns，且涉及 IOMMU 时更贵）。

> 详见 [chapter-04 page_pool](../../chapter-04-page-pool/)。

---

## 六、观测：把契约变成可查的数字

```bash
# 队列是否频繁 stop/wake（→ requeues 的根源）
tc -s qdisc show dev eth0          # 看 requeues / backlog / dropped

# 驱动层计数器（驱动自己定义的名字，各家不同）
ethtool -S eth0 | grep -iE "tx_|rx_"
#   重点：tx_busy / tx_restart / no_buf / missed / rx_dropped

# 中断合并（决定收包延迟和完成通知延迟）
ethtool -c eth0
ethtool -C eth0 rx-usecs 0 tx-usecs 0      # 低延迟：全关
ethtool -C eth0 adaptive-rx off            # 自适应合并会自己把延迟加上去

# 队列 stop 的直接证据（tracepoint）
perf trace -e 'net:net_dev_xmit_timeout'   # Tx 超时
bpftrace -e 'tracepoint:net:net_dev_queue { @[comm] = count(); }'

# 丢包定位（5.15+ 带 reason）
bpftrace -e 'tracepoint:skb:kfree_skb { @[args->reason] = count(); }'
```

---

## HFT 要点

- **`NETDEV_TX_BUSY` 不是正常的背压机制**，是驱动兜底。看到 `requeues` 涨，要去调 Tx ring 大小和 stop 阈值，不是"接受它"。
- **队列 stop/wake 才是零成本流控**：`netif_tx_stop_queue()` 让 qdisc 根本不 dequeue，比"dequeue 了再返回 BUSY 让内核 requeue"便宜得多。
- **释放函数选错会让监控说谎**：完成路径用 `napi_consume_skb(skb, budget)`，别用 `kfree_skb()`——后者会把每个成功的包记成丢包。
- **`napi_consume_skb` 的 `budget` 参数是"是否 NAPI 上下文"的标记**，非零才走 per-CPU 缓存。名字极具误导性。
- **克隆过的包释放不走缓存**，更贵。高 PPS 下能省则省。
- **中断合并同时影响收发两端**：`rx-usecs` 决定收包延迟，`tx-usecs` 决定完成通知延迟（进而决定 `MSG_ZEROCOPY` 的 buffer 周转）。
- **budget 越大延迟越"尖"**：单轮 softirq 太长会推迟其他核。低延迟要小 budget。
- **page_pool 的保持映射**是收发循环里最容易被忽略的一笔节省（省掉每次 DMA map/unmap）。

## 与 Rosen 3.x 的差异

| Rosen 3.x（2.6 时代） | 现在（5.x/6.x） |
|----------------------|----------------|
| `NETDEV_TX_LOCKED` 是有效返回值 | **已移除**，`netdev_tx` 只剩 OK / BUSY |
| Rx buffer 用 `alloc_page` | `page_pool`（可保持 DMA 映射） |
| 驱动直接 `netif_receive_skb()` | 必须 `napi_gro_receive()` |
| 无 XDP | native XDP 在 skb 构造之前 |
| 释放 skb 用 `dev_kfree_skb` / `kfree_skb` | NAPI 上下文用 `napi_consume_skb()` |
| 丢包只能靠计数器猜 | 5.15+ `kfree_skb_reason` tracepoint 直接给出原因 |
| 单队列 | 多队列 + per-queue `napi_struct` |
| GRO 仅软件 | 还有硬件 GRO（`rx-gro-hw`），但会让 tcpdump 看到假包 |

> 关于硬件 GRO 的陷阱，见 [chapter-02/04-gro-gso](../../chapter-02-napi-rx-path/notes/04-gro-gso.md)。

---

## 代码自测

<details>
<summary>Q1：你在 <code>dropwatch</code> 里看到每秒几万次 <code>kfree_skb</code> 事件，但业务没报丢包、对端也正常收到。最可能的原因是什么？</summary>

<b>答：</b>首先怀疑<b>释放函数用错了</b>，而不是真丢包。

`kfree_skb()` 走 `SKB_DROP_REASON_NOT_SPECIFIED`，会被 drop_monitor 记录；而 `consume_skb()` / `napi_consume_skb()` 走 `SKB_CONSUMED`，属于正常投递、不记录。

如果某个驱动（或你自己写的 BPF/内核模块）在完成路径上误用了 `kfree_skb()`，那么<b>每一个成功发出的包都会被记成一次丢包</b>——数量级恰好和 PPS 吻合。

验证方法：
```bash
bpftrace -e 'tracepoint:skb:kfree_skb { @[kstack] = count(); }'
```
看调用栈来自哪里。如果来自驱动的 Tx 清理路径，就是误用；如果来自 `udp_rcv` / socket 队列溢出之类，才是真丢包。

<b>判断顺序很重要：先看栈，再下结论。</b>
</details>

<details>
<summary>Q2：<code>tc -s qdisc show</code> 显示 <code>requeues</code> 持续上涨。这代表什么？该怎么处理？</summary>

<b>答：</b>`requeues` 上涨说明 `ndo_start_xmit()` 返回了 `NETDEV_TX_BUSY`（0x10），导致 `dev_xmit_complete(rc)` 判定为 false，内核把 skb 重新入队再调度一次。

<b>这是纯浪费的延迟</b>——本来正确的做法是驱动在 Tx ring 快满时调用 `netif_tx_stop_queue(txq)`，让 qdisc 层根本不 dequeue；完成中断里再 `netif_tx_wake_queue(txq)` 恢复。stop/wake 是零成本的，BUSY + requeue 是"已经走到门口了又退回来"。

处理：
1. 增大 Tx ring：`ethtool -G eth0 tx 4096`（原值用 `ethtool -g eth0` 查）
2. 检查驱动的 stop 阈值是否合理（有些驱动 ring 剩 1/8 才 stop，太晚）
3. 降低 burst：如果应用一次性塞太多包，考虑分批

注意：requeues 发生在"队列将满未满"的边界，这正是发包延迟分布<b>第二个峰</b>的来源——不可预测，危害比稳定的排队更大。
</details>

<details>
<summary>Q3：驱动的 Tx 清理函数里，把 <code>napi_consume_skb(skb, budget)</code> 改成 <code>napi_consume_skb(skb, 0)</code>，性能会变差吗？为什么？</summary>

<b>答：</b>会变差，而且是两方面：

1. **丢了 per-CPU 缓存**。`budget == 0` 时函数直接 `dev_consume_skb_any(skb)` 返回，走不到底部的 `napi_skb_cache_put(skb)`。于是每个 skb 头都要走一次 slab 分配器，而不是从 per-CPU 缓存拿/放。高 PPS 下这是实打实的 cache miss 和锁竞争。

2. **语义变了**。`budget` 在这里的真实含义是"你是否在 NAPI 上下文"，不是"还剩多少预算"。传 0 等于告诉内核"我不在 NAPI 里"，于是走非 NAPI 路径——连 `DEBUG_NET_WARN_ON_ONCE(!in_softirq())` 的检查都绕过去了，掩盖了真实问题。

这个参数名确实极具误导性：<b>它长得像预算，实际是个布尔标记。</b>
</details>

---

→ 前一篇：[01 发包路径](01-tx-path-bootlin.md)
→ 后一篇：[03 sk_buff 生命周期](03-sock-sk-buff.md)
→ 相关：[chapter-04 page_pool](../../chapter-04-page-pool/) · [chapter-02 NAPI](../../chapter-02-napi-rx-path/notes/02-napi.md)
