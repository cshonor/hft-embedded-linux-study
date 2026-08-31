# 13-01 — Receive/Transmit Packet Steering：RSS / RPS / RFS / aRFS / XPS / SO_INCOMING_CPU（v6.6 源码级）

> **对应 Rosen:** Ch14（高级主题）
> **内核源码路径:** `Documentation/networking/scaling.rst`、`net/core/dev.c`、`net/core/sock.c`、`net/core/sock_reuseport.c`

## 章节导航

| 上一篇 | 本篇 | 下一篇 |
|---|---|---|
| [12-io-uring-net](../../chapter-12-io-uring-net/README.md) | **13-01 scaling** | [13-02 MSG_ZEROCOPY](02-msg-zerocopy.md) |

## 本节讲什么

上一章 io_uring 解决的是**单个线程内部的系统调用开销**；本节解决的是**包与 CPU 的物理分布问题**——一个 skb 从网卡 DMA 到达、到最终被应用层 `recvmsg` 读走，中间经历的所有"该在哪个 CPU 上处理"的决策点。这是多核扩展（scaling）的完整链路：

```
网卡硬件 RSS ──► RX 队列选择
                   │
                   ▼
           netif_receive_skb_internal()     dev.c:5730
                   │
                   ▼
           get_rps_cpu()                    dev.c:4557
           ├── RFS：flow→CPU 全局表命中？
           │      └── 命中 → enqueue_to_backlog() 到目标 CPU（IPI）
           └── RPS：仅按 hash 分发
                   │
                   ▼
           TCP 层 → socket 锁 → 应用 recvmsg
                   │
                   ▼
           reuseport_select_sock_by_hash()   sock_reuseport.c:528
           （SO_REUSEPORT + incoming_cpu 联动的最终收口）
```

发送侧对称的问题是 XPS：**哪个 CPU 允许使用哪个 TX 队列**。

## 要点（先记住结论）

1. **RSS 是硬件、RPS 是软件、RFS 是带亲和性的 RPS、aRFS 是 RFS 的硬件加速版**——四者不是并列选项，而是同一决策链上可叠加的层。
2. **RPS 的分发现场在 `get_rps_cpu()`（dev.c:4557）**：先查 RFS 全局流表 `rps_sock_flow_table`（dev.c:4497），未命中才退回按 `skb_get_hash()` 纯 hash 分发。两张表都为空时函数直接 `goto done` 返回 -1，**零开销通过**。
3. **RPS 的代价是 IPI（处理器间中断）+ `enqueue_to_backlog()`（dev.c:4788）的入队锁**：`rps_lock_irqsave`（dev.c:220）保护目标 CPU 的 `softnet_data` backlog 队列。跨 NUMA 节点的 cache miss + IPI 延迟是微秒级——对 HFT 这是不可忽视的成本。
4. **RFS 的核心洞察**：纯 hash RPS 会把同一 flow 的包分到固定 CPU，但**消费该 socket 的应用线程可能跑在另一个 CPU 上**，导致 socket 锁跨核、skb 数据 cache-cold。RFS 用 `rps_sock_flow_table` 记录"上次 `recvmsg` 发生在哪个 CPU"，让包追着消费者走。
5. **`SO_INCOMING_CPU`（sock.c:1451 设置 / :1924 读取）+ accept 时的 `newsk->sk_incoming_cpu = raw_smp_processor_id()`（sock.c:2385）**：内核在 accept 时自动给新 socket 打上"出生 CPU"标记，配合 `reuseport_select_sock_by_hash()` 的 incoming_cpu 匹配（sock_reuseport.c:528），实现 O(1) 的 CPU 亲和 accept 调度。
6. **XPS 是发送侧镜像**：`/sys/class/net/eth0/queues/tx-*/xps_cpus` 限定"CPU→TX 队列"映射，避免多 CPU 争抢同一 TX 队列的 `__netif_tx_lock`，同时保证 skb 在构造它的同一 CPU 上完成 DMA（cache 友好）。

## 一、RSS：硬件 hash，问题的第一层

多队列网卡（RSS，Receive Side Scaling）在硬件中对每个到达的包计算 4-tuple hash（Intel 称 RSS hash），通过 indirection table（间接表）映射到某个 RX 队列。**每个 RX 队列绑定一个 MSI-X 中断，中断亲和性决定包在哪个 CPU 触发 NAPI 软中断**。

```
ethtool -x eth0          # 查看 indirection table
ethtool -X eth0 equal 4  # 均匀分发到 4 个队列
ethtool -L eth0 combined 4   # 4 个 RX/TX 组合队列
```

RSS 的局限：
- hash 输入固定为 4-tuple，**无法感知应用层负载**——行情组播流量全落同一个 flow（同 group 同 port），单队列打满，其他队列空闲。
- 硬件表大小有限（通常 128/512 项），且对 jumbo frame / 分段包 hash 质量下降。

**HFT 视角**：交易所行情走组播，同一 multicast group 的所有流量 4-tuple 相同 → RSS 彻底失效，**必须靠多播组级别分流（网卡 ntuple 规则把不同 group 指到不同队列）或 XDP CPUMAP**。

## 二、RPS：软件 steering，`get_rps_cpu()` 源码级

RPS 在 `netif_receive_skb_internal()`（dev.c:5730）之后的 `process_context` 里调用 `get_rps_cpu()`。先看判空快速路径：

```c
// dev.c:4557 get_rps_cpu() 开头（节选）
flow_table = rcu_dereference(rxqueue->rps_flow_table);
map = rcu_dereference(rxqueue->rps_map);
if (!flow_table && !map)
	goto done;              // ← 没配 RPS/RFS：直接返回 -1，零开销
```

**关键点：RPS/RFS 完全是 opt-in 的**。不配置任何 `rps_cpus` 时，这段代码对性能零影响（一次 RCU 读 + 两次判空）。

配置后，hash 计算用的是 `skb_get_hash()`（基于 flow dissector 的通用 hash，不依赖网卡硬件 hash 值）：

```c
skb_reset_network_header(skb);
hash = skb_get_hash(skb);
if (!hash)
	goto done;
```

### RPS 分发目标的选择

纯 RPS（没配 RFS）时目标 CPU = `map->cpus[hash % map->len]`——按 rxqueue 的 `rps_map`（来自 sysfs 的 `rps_cpus` 掩码）做取模选择。

### `enqueue_to_backlog()`：跨 CPU 递送的现场

```c
// dev.c:4788（节选）
static int enqueue_to_backlog(struct sk_buff *skb, int cpu,
			      unsigned int *qtail)
{
	struct softnet_data *sd;
	unsigned long flags;
	unsigned int qlen;

	sd = &per_cpu(softnet_data, cpu);       // ← 目标 CPU 的 backlog
	rps_lock_irqsave(sd, &flags);           // ← dev.c:220，per-CPU 队列锁
	if (!netif_running(skb->dev))
		goto drop;
	qlen = skb_queue_len(&sd->input_pkt_queue);
	if (qlen <= netdev_max_backlog && !skb_flow_limit(skb, qlen)) {
		if (skb_queue_len(&sd->input_pkt_queue)) {
enqueue:
			__skb_queue_tail(&sd->input_pkt_queue, skb);
			...
			return NET_RX_SUCCESS;
		}
		/* Schedule NAPI for backlog queue */
		____napi_schedule(sd, &sd->backlog);  // ← 目标 CPU 上排软中断
		...
	}
	...
}
```

成本清单（跨 CPU RPS 一跳的实际代价）：
1. 目标 CPU `softnet_data` 队列锁（目标 CPU 上如果也在收包，有争抢）；
2. `____napi_schedule` 触发 **IPI**（`smp_call_function_single` 路径），唤醒目标 CPU 的 softirq；
3. skb 数据本身 cache-cold——它在源 CPU 的 cache 里是热的，到目标 CPU 要重新拉 line；
4. backlog 队列有 `netdev_max_backlog`（默认 1000）上限，超限直接 drop——**`/proc/net/softnet_stat` 第二列增长 = RPS 溢出丢包**。

## 三、RFS：让包追着消费者走

### 全局流表 `rps_sock_flow_table`

```c
// dev.c:4497
struct rps_sock_flow_table __rcu *rps_sock_flow_table __read_mostly;
```

这是一个全局（非 per-queue）的 flow→CPU 表，大小由 `/proc/sys/net/core/rps_sock_flow_entries` 控制。**写入时机**：应用调用 `recvmsg`/`sendmsg` 时，`sock_rps_record_flow()` 把「flow hash → 当前 CPU」记入表中（`lockless` 的 `WRITE_ONCE`，见 `rps_record_sock_flow()`，与 `get_rps_cpu()` 里的 `READ_ONCE(ident)` 配对）。

### 查表逻辑（get_rps_cpu 中段）

```c
// dev.c:4557 函数体节选
ident = READ_ONCE(sock_flow_table->ents[hash & sock_flow_table->mask]);
if ((ident ^ hash) & ~rps_cpu_mask)
	goto try_rps;            // ← 表项不属于这个 flow（陈旧/冲突）→ 退回纯 RPS

next_cpu = ident & rps_cpu_mask;
```

表项把 hash 的高位存进 ident 字段做"属主校验"，避免 hash 冲突时错配。校验通过后还要过第二张表——**per-RX-queue 的 `rps_flow_table`**（设备侧），其表项记录"上次实际分发的 CPU"（tcpu），当且仅当满足以下条件才切换到新 CPU：目标 CPU 未设置 / offline / **旧 CPU 的 backlog 尾部已推进到该 flow 项之后**（保证同 flow 的包不乱序）。这就是文档里著名的 anti-reordering 约束。

### aRFS：硬件加速 RFS

RFS 仍是软件分发（IPI 已经发生）。aRFS（accelerated RFS）把决策前推到网卡：驱动实现 `ndo_rx_flow_steer()`（netdevice.h:1531），内核在 `get_rps_cpu()` 路径中调用它（dev.c:4533），驱动把 ntuple 规则写入网卡，**让后续同 flow 的包直接由硬件投递到目标 RX 队列**（该队列的中断已绑在消费者 CPU 上）——IPI 消失，包天生落在正确的 CPU。

陈旧规则的回收：驱动周期性调用 `rps_may_expire_flow()`（dev.c:4667），判断某 ntuple filter 是否已无对应的活跃流表项，可安全删除。 Mellanox（mlx4/mlx5）与部分 Chelsio 网卡支持。

```bash
# aRFS 前提：打开网卡 ntuple
ethtool -K eth0 ntuple on
echo 32768 > /proc/sys/net/core/rps_sock_flow_entries
# per-queue flow count = 全局 entries / 队列数
```

## 四、XPS：发送侧镜像

TX 队列选择函数 `netdev_pick_tx()` 的逻辑：如果配置了 XPS（`tx-*/xps_cpus`），从「当前 CPU 允许的队列集合」中按 `skb_get_hash` 或 round-robin 选一个；没配置则在**所有队列**间 round-robin——多 CPU 同时发包时会集中争抢同一把 `__netif_tx_lock`（qdisc lock），且 `skb` 在 CPU A 构造、在 CPU B 的队列里 DMA，cache 全程冰冷。

```bash
# 4 队列 × 4 CPU 一对一绑定（HFT 典型）
for i in 0 1 2 3; do
  echo $((1 << i)) > /sys/class/net/eth0/queues/tx-$i/xps_cpus
done
```

XPS 与 RPS 的对称性：

| | RPS | XPS |
|---|---|---|
| 方向 | RX | TX |
| 配置 | `queues/rx-*/rps_cpus` | `queues/tx-*/xps_cpus` |
| 选择依据 | flow hash + RFS 流表 | 当前 CPU 掩码 + hash |
| 收益 | 软中断负载分散 | 队列锁不争抢 + DMA cache 亲和 |

## 五、`SO_INCOMING_CPU` + SO_REUSEPORT：调度层的收口

前四层解决"包到 CPU"，最后一层解决"CPU 上的多个 listen socket 归谁"。这是 v4.6+ 加入（Facebook 贡献）的机制，三条源码锚点：

```c
// sock.c:1451  —— setsockopt(SO_INCOMING_CPU, n)
case SO_INCOMING_CPU:
	ret = -EPERM;
	if (!sockopt_capable(CAP_NET_ADMIN))
		break;
	ret = -EINVAL;
	if (val < -1 || val >= nr_cpu_ids)
		break;
	write_lock_bh(&sk->sk_callback_lock);
	if (!sock_flag(sk, SOCK_INCOMING_NAPI_ID)) { ... }
	WRITE_ONCE(sk->sk_incoming_cpu, val);
	...

// sock.c:2385 —— accept 时新 socket 继承"出生 CPU"
newsk->sk_incoming_cpu = raw_smp_processor_id();
```

**默认值 -1**（sock.c:3479 初始化），表示"未标记"。

### `reuseport_select_sock_by_hash()` 的完整逻辑（sock_reuseport.c:528）

```c
static struct sock *reuseport_select_sock_by_hash(struct sock_reuseport *reuse,
						  u32 hash, u16 num_socks)
{
	struct sock *first_valid_sk = NULL;
	int i, j;

	i = j = reciprocal_scale(hash, num_socks);   // ← hash → [0, num_socks)
	do {
		struct sock *sk = reuse->socks[i];

		if (sk->sk_state != TCP_ESTABLISHED) {
			if (!READ_ONCE(reuse->incoming_cpu))
				return sk;                       // ① 无 CPU 亲和要求
			if (READ_ONCE(sk->sk_incoming_cpu) == raw_smp_processor_id())
				return sk;                       // ② 本 CPU 专属 socket
			if (!first_valid_sk)
				first_valid_sk = sk;             // ③ 记住第一个可用者
		}
		i++;
		if (i >= num_socks)
			i = 0;
	} while (i != j);

	return first_valid_sk;                        // ④ 全不匹配 → 退化为轮询
}
```

逐条解读：
- **`reciprocal_scale(hash, num_socks)`**：用乘法+移位代替除法取模（`(u64)hash * ep_ro >> 32`），避免 div 指令的 ~20-40 cycle 开销——热路径细节。
- **② 命中条件**：`sk_incoming_cpu == 当前 CPU`。注意 `reuse->incoming_cpu` 是个计数器（sock_reuseport.c:40-61），统计组内有多少个 socket 设置了 CPU 亲和——**组内一个都没设时走 ① 直接返回，保持纯 hash 行为**。
- **④ 兜底**：本 CPU 的专属 socket 全部 ESTABLISHED（忙），从 hash 起点轮询找第一个非 ESTABLISHED 的——保证 accept 不饿死，只是失去亲和。

### accept-retry 编程范式

配合 `SO_INCOMING_CPU` 的经典服务端写法（每个工作线程一个 listen socket）：

```c
// 每线程：
int fd = socket(...);
setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, ...);
bind(fd, ...); listen(fd, ...);

// 获取"这个 socket 被内核调度在哪个 CPU"（读 sock.c:1924 的 get 路径）
int cpu;
socklen_t len = sizeof(cpu);
getsockopt(fd, SOL_SOCKET, SO_INCOMING_CPU, &cpu, &len);

// CPU 不对就关掉重来，直到内核把 fd 放到本线程的 CPU 上
while (cpu != sched_getcpu()) {
    close(fd);
    fd = recreate_listen_socket();
    getsockopt(fd, SOL_SOCKET, SO_INCOMING_CPU, &cpu, &len);
}
// 此后：accept 出的连接的 sk_incoming_cpu == 本 CPU（sock.c:2385），
// 软中断、TCP 处理、应用读全在同核。
```

## 六、五层机制总对比

| 层 | 机制 | 决策者 | 粒度 | 需要硬件支持 | 配置接口 |
|---|---|---|---|---|---|
| 1 | RSS | 网卡 | 4-tuple | ✅ 多队列网卡 | `ethtool -X` |
| 2 | RPS | 内核软件 | flow hash | ❌ | sysfs `rps_cpus` |
| 3 | RFS | 内核软件 | flow→消费者 CPU | ❌ | `rps_sock_flow_entries` |
| 3+ | aRFS | 网卡 ntuple | flow→队列 | ✅ 驱动支持 | `ethtool -K ntuple on` |
| 4 | SO_INCOMING_CPU + reuseport | 内核调度 | socket→CPU | ❌ | setsockopt |
| TX | XPS | 内核软件 | CPU→TX 队列 | ❌ | sysfs `xps_cpus` |

## 七、vs XDP CPUMAP

| 维度 | RPS/RFS | XDP CPUMAP |
|---|---|---|
| 分发时机 | sk_buff 分配**之后** | sk_buff 分配**之前**（XDP 程序里） |
| CPU 开销 | 分配 skb + 队列锁 + IPI | 轻量：ptr 入 per-CPU ring |
| 亲和性 | RFS 跟踪 recvmsg CPU | BPF 自定义（任意字段） |
| 灵活性 | hash 固定 4-tuple | BPF 任意逻辑（可解析 payload） |
| 功能 | 只搬运 | 搬运 + 可就地修改/丢弃 |

细节：CPUMAP 转发用的也是 `enqueue_to_backlog` 同款 per-CPU backlog 机制（`cpu_map_enqueue`），但因为发生在 skb 构造之前，省掉了丢弃路径的 skb 分配成本。

## HFT 要点

- **行情接收（组播）**：RSS 因 4-tuple 相同而失效 → 用 ntuple 规则（`ethtool -N`）按组播组分流到 RX 队列，或直接 XDP CPUMAP（可按 payload 里的 symbol/channel 分流，比 ntuple 灵活）。
- **交易发送**：XPS 一对一绑定，确保 `sendmsg` → qdisc → DMA 全程同一 CPU，TX 队列锁零争抢。
- **RFS 对 HFT 意义有限**：前提是包走完整协议栈；HFT 行情走 XDP/AF_XDP 时根本不经过 `get_rps_cpu()`。
- **终极形态是"全链路同核"**：RSS/ntuple（包到对 CPU）+ `SO_INCOMING_CPU`（socket 到对 CPU）+ `sched_setaffinity`（线程绑核）+ XPS（发包不跨核）——中间任何一环断裂，就是一次跨核 cache miss 或 IPI。
- 监控：`/proc/net/softnet_stat`（第 1 列 packet 处理数、第 2 列 drop、第 3 列 squeeze）；`ss -tma` 看连接的 napi_id/incoming_cpu。

## 衔接

本篇解决"包和 CPU 怎么对齐"。下一篇 [13-02 MSG_ZEROCOPY](02-msg-zerocopy.md) 解决"对齐之后，数据还要在内核态与应用态之间搬运几次"——发送方向的用户态零拷贝（`skbuff.c:1540` 的 `msg_zerocopy_alloc` / page pin / completions 通知链）。

## 代码自测

<details>
<summary>Q1：为什么说"RPS 在有 RSS 的机器上通常是负优化"？什么情况下例外？</summary>

RSS 让包从硬件层面就落在中断亲和指定的 CPU 上，数据一路 cache 热。RPS 在软件层把包再搬到另一个 CPU，白白付出 IPI + cache miss。例外：
1. 单队列网卡 + 多核机器（无 RSS 可用）；
2. RSS hash 失效场景（如全部流量同 4-tuple）且配合 RFS 让包跟随消费者；
3. 需要把 RX 软中断从实时核上挪走（如行情核不跑 softirq，包转到其他核处理，代价换隔离）。
</details>

<details>
<summary>Q2：`get_rps_cpu()` 里 RFS 表项的 `(ident ^ hash) & ~rps_cpu_mask` 校验在防什么？</summary>

防 hash 冲突误配。表项用 `hash & mask` 做索引，两个不同 flow 可能落在同一表项。表项把原始 hash 的**高位**（~rps_cpu_mask 的部分）存下来，查表时比对——不一致说明表项属于另一个 flow（或已陈旧），直接 `goto try_rps` 退回纯 hash 分发，不会把包送到别的 flow 的 CPU 上。
</details>

<details>
<summary>Q3：RFS 为什么需要 per-queue 的第二张表（rps_flow_table）？只有全局表行不行？</summary>

只有全局表会产生**乱序**：全局表说"新 CPU"，但旧 CPU 的 backlog 里可能还压着同 flow 的前几个包，直接切换会导致后包先到 TCP 层。per-queue 表项记录 `last_qtail`——只有当旧 CPU 的队列尾部已推进越过该 flow 上次入队的位置（旧包已消费完），才允许切换到新 CPU。这是 RFS 文档里 anti-reordering 机制的核心。
</details>

<details>
<summary>Q4：`reuseport_select_sock_by_hash()` 为什么先检查 `!READ_ONCE(reuse->incoming_cpu)` 就直接返回，而不是检查单个 socket？</summary>

`reuse->incoming_cpu` 是**组内设置了 CPU 亲和的 socket 计数**（sock_reuseport.c:40-61 的 get/put 对维护）。组内一个都没设置时（计数为 0），逐个检查 `sk_incoming_cpu == 当前CPU` 毫无意义（都是 -1），直接返回 hash 选中的 socket 保持纯 hash 语义——这是一次 O(1) 的整组短路，避免热路径浪费 N 次比较。
</details>

<details>
<summary>Q5：XPS 没配置时，多线程发包的性能损失具体在哪两个环节？</summary>

1. **TX 队列锁争抢**：`netdev_pick_tx()` 无 XPS 时在全部队列间 round-robin，多个 CPU 会撞到同一队列的 `__netif_tx_lock`（qdisc 入队锁），产生自旋等待；
2. **DMA cache miss**：skb 在 CPU A 构造（数据在 A 的 L1/L2 里是热的），却从 CPU B 关联的 TX 队列做 DMA，网卡读内存时数据对 LLC 而言位置随意——配置 XPS 后包从构造到 DMA 全程同核，数据始终 cache 热。
</details>
