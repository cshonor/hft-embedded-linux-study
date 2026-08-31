# 13-05 — SO_REUSEPORT：内核级多监听者分发与 eBPF 选择器（v6.6 源码级）

> **对应 Rosen:** 无（3.x 时代机制，Rosen 未覆盖）
> **内核源码路径:** `net/core/sock_reuseport.c`、`include/uapi/linux/bpf.h:6355`（`sk_reuseport_md`）

## 章节导航

| 上一篇 | 本篇 | 下一篇 |
|---|---|---|
| [13-04 接收侧 ZC](04-tcp-zero-copy-recv.md) | **13-05 SO_REUSEPORT** | [chapter-14 TCP/UDP 内部机制](../chapter-14-tcp-udp-internals/README.md) |

## 本节讲什么

本模块前四篇都在优化"一条数据路径"的效率；本篇优化"**多条数据路径的入口分布**"：多个进程/线程监听同一端口时，连接（或数据报）如何到达正确的消费者。SO_REUSEPORT（3.9+）把分发从用户态 master 进程下沉到内核 lookup 路径里，4.6+ 又允许 eBPF 接管选择逻辑，5.14+ 进一步支持请求迁移。它还是 [13-01](01-scaling.md) 里 `SO_INCOMING_CPU` + `reuseport_select_sock_by_hash()` 那条 CPU 亲和链的最后一环。

## 要点（先记住结论）

1. **REUSEPORT 组是内核里的一个 RCU 保护数组**：`struct sock_reuseport`（sock_reuseport.c）持有 `socks[]`、BPF prog 指针和 `incoming_cpu` 计数器；`bind()` 时 `reuseport_alloc()`（:189）建组，满了 `reuseport_grow()`（:247）翻倍扩容并剔除已关闭的成员。
2. **分发现场在协议栈 lookup 路径**（TCP `__inet_lookup_listener`、UDP `__udp4_lib_lookup`）：命中组后调 `reuseport_select_sock()`（sock_reuseport.c:569）——**BPF 优先，hash 兜底**。
3. **两种 BPF 程序类型**：`BPF_PROG_TYPE_SK_REUSEPORT`（专为此设计，返回值 = 组内 socket 索引）和 legacy socket filter（返回 0..N-1 同语义，靠 `run_bpf_filter` 解释）。BPF 返回无效值（越界/负）时静默退回 hash 选择。
4. **`sk_reuseport_md` 上下文从传输层头部开始**（uapi bpf.h:6355）：`data/data_end` 指向 TCP/UDP header，另有 `len/eth_protocol/ip_protocol/bind_inany/hash` 和两个 sock 指针——**BPF 可以看 4-tuple 之外的东西**（如 TCP 选项、payload 前几字节）做路由决策。
5. **TCP listener 迁移（5.14+，`reuseport_migrate_sock()` :621）**：组内某 listen socket 关闭时，其未完成的握手（reqsk）和已建立连接可被 BPF 程序"改嫁"到组内其他 socket——滚动重启 worker 不丢连接。
6. **UDP 组播与单播语义不同**：单播数据报走 `reuseport_select_sock` 选**一个**；组播则是**每个加入了该组的 socket 各得一份拷贝**——多进程行情分发的免费 fan-out。

## 一、没有 REUSEPORT 的世界

- **`SO_REUSEADDR` ≠ `SO_REUSEPORT`**：REUSEADDR 只允许 TIME_WAIT 重绑定，同端口并发监听仍冲突（除组播）。
- 旧方案 A：master 进程 accept 后 fd 传递给 worker（SCM_RIGHTS）——master 是单点，两次上下文切换。
- 旧方案 B：多进程阻塞在同一 listen fd 上 accept——内核唤醒所有等待者（惊群），N-1 个抢空而归；`WQ_FLAG_EXCLUSIVE` 缓解但不消除。
- 旧方案 C：epoll 共享 listen fd + EPOLLEXCLUSIVE——事件分发仍单点，负载不均。

REUSEPORT 把选择权**直接放进包的 lookup 路径**：每个 worker 自己的 listen socket 都在内核 hash 表里，SYN 到达时当场决定走谁的队列——没有 master、没有惊群、没有额外切换。

## 二、组的管理：alloc / grow / detach

```
bind() 时 sk_reuseport=1 且地址已被其他同 flag socket 占用
   → 加入对方的 reuseport 组（或建组）
   → __reuseport_add_sock()：socks[num_socks++] = sk，smp_wmb() 发布
close(sk)
   → reuseport_detach_sock()：摘除；组空则释放
   → 有 BPF prog 时先通知（REUSEPORT_SOCKARRAY map 里的引用失效）
```

`reuseport_grow()`（:247）的细节：扩容翻倍新数组、RCU 替换；**顺手剔除已关闭的 socket**（`RCU_INIT_POINTER(sk->sk_reuseport_cb, NULL)`，:262）——`INIT_SOCKS`（初始容量）之后按 2 的幂增长。`__reuseport_add_sock` 的 `smp_wmb()` 与 `reuseport_select_sock` 里的 `smp_rmb()` 配对：保证选择者看到的 `num_socks` 增大时，`socks[]` 内容必然已就绪。

## 三、分发核心：`reuseport_select_sock()`（:569）

```c
struct sock *reuseport_select_sock(struct sock *sk, u32 hash,
				   struct sk_buff *skb, int hdr_len)
{
	reuse = rcu_dereference(sk->sk_reuseport_cb);
	if (!reuse) goto out;                       // 组还没建好（竞态窗口）

	prog = rcu_dereference(reuse->prog);
	socks = READ_ONCE(reuse->num_socks);
	if (likely(socks)) {
		smp_rmb();                              // 与 add_sock 的 wmb 配对

		if (!prog || !skb)
			goto select_by_hash;

		if (prog->type == BPF_PROG_TYPE_SK_REUSEPORT)
			sk2 = bpf_run_sk_reuseport(reuse, sk, prog, skb, NULL, hash);
		else
			sk2 = run_bpf_filter(reuse, socks, prog, skb, hdr_len);

select_by_hash:
		if (!sk2)                               // 无 BPF 或 BPF 给了无效结果
			sk2 = reuseport_select_sock_by_hash(reuse, hash, socks);
	}
	...
}
```

关键设计：**BPF 永远有 hash 兜底**。BPF 程序崩了逻辑（返回越界索引）、或 attach 被摘掉的瞬间，数据路径不中断——最多退化为 hash 分发。这符合"BPF 增强而不接管关键路径"的一贯哲学（对照 chapter-05 XDP 的 verdict 兜底）。

`reuseport_select_sock_by_hash()` 的逐行解读（`reciprocal_scale` + incoming_cpu 匹配 + 兜底轮询）在 [13-01 第五节](01-scaling.md)——那里是把它当"CPU 亲和收口"讲的，这里补充分发视角：**hash 保证 per-flow 一致性**（同一客户端总是到同一 worker，TCP 序列/状态不分裂），incoming_cpu 修饰只影响"同 flow 重选"场景。

## 四、eBPF 选择器：`sk_reuseport_md` 上下文

```c
// include/uapi/linux/bpf.h:6355
struct sk_reuseport_md {
	void *data;          // ← 从 TCP/UDP 头开始
	void *data_end;
	__u32 len;           // 从传输层头起的总长
	__u32 eth_protocol;
	__u32 ip_protocol;
	__u32 bind_inany;
	__u32 hash;          // 4-tuple hash（内核算好的）
	struct bpf_sock *sk;          // 组内当前被 lookup 的 socket
	struct bpf_sock *migrating_sk; // 迁移场景下的原 socket
};
```

**返回值即选择**：程序返回 `0..num_socks-1` 的索引。更工程化的写法是配合 `BPF_MAP_TYPE_REUSEPORT_SOCKARRAY` map + `bpf_sk_select_reuseport()` helper（uapi bpf.h:3503）——map 里存 socket 引用，程序按业务逻辑查 map 选 socket，worker 动态增减时只需更新 map（内核会校验所选 socket 确实在组里且匹配）。

```c
/* 典型：按 payload 首字段把行情分给对应 symbol-owner worker */
SEC("sk_reuseport")
int pick_worker(struct sk_reuseport_md *ctx)
{
	__u32 symbol_id;
	if (bpf_skb_load_bytes(ctx, 0, &symbol_id, sizeof(symbol_id)))
		return 0;                        // 读不到 → 索引 0 兜底
	__u32 idx = symbol_id % NUM_WORKERS;
	if (bpf_sk_select_reuseport(ctx, &sock_arr, &idx, 0))
		return 0;                        // helper 失败 → 兜底
	return idx;
}
```

注意旧版笔记里 `return cpu % NUM_SOCKETS` 的写法是**错的**：返回值是组内索引不是 CPU 号；想按 CPU 选要么维护"CPU→索引"映射（结合 `bpf_get_smp_processor_id`），要么干脆用 [13-01](01-scaling.md) 讲的 `SO_INCOMING_CPU` 路线（内核自动，无 BPF）。

## 五、TCP listener 迁移（5.14+）

`reuseport_migrate_sock()`（:621）在两种时机被调：
1. **listen socket 关闭**：其 accept 队列里的 reqsk（半连接）与已建连 socket 被拿出来重新选主；
2. 对应 BPF 上下文里 `migrating_sk != NULL`——程序可以感知"这是迁移不是新连接"，按连接属性（而非新 SYN 的 hash）选择目标。

生产意义：**滚动重启 worker 不丢连接**。老内核里关闭 listen socket 会杀掉整个 accept 队列（半连接直接 RST）；有了迁移 + BPF 自定义逻辑，连接被平移到其他 worker，客户端无感。

## 六、UDP：单播选一 vs 组播全发

| 流量 | 行为 |
|---|---|
| UDP 单播 | `__udp4_lib_lookup` → reuseport_select_sock → hash/BPF 选**一个** socket |
| UDP 组播 | 遍历所有 join 该 (group, port) 的 socket，**每个都拷贝一份**投递 |

对 HFT 行情组播：N 个策略进程各自 `bind + IP_ADD_MEMBERSHIP`，网卡只收一份、内核复制 N 份（`udpv4_mcast_deliver` 路径）——进程间隔离零共享，代价是每进程一次 skb clone。当 N 大且单份成本敏感时，才值得换"单进程收 + 共享内存分发"架构（这是 chapter-06 AF_XDP 后处理的话题）。

## 七、惊群与分发机制对比

| 机制 | 分发者 | 惊群 | per-flow 一致性 | 可编程 |
|---|---|---|---|---|
| master accept + fd 传递 | 用户态 | 无 | 无（accept 顺序） | ✅ 完全 |
| 多进程阻塞 accept | 内核唤醒 | **有**（WQ_FLAG_EXCLUSIVE 缓解） | 无 | ❌ |
| epoll + EPOLLEXCLUSIVE | 内核事件 | 消除 | 无 | ❌ |
| SO_REUSEPORT（hash） | 内核 lookup | 无 | **有**（4-tuple hash） | ❌ |
| SO_REUSEPORT + BPF | 内核 lookup | 无 | 自定义 | ✅ |

## HFT 关联

- **TCP 行情/交易网关**：多 worker REUSEPORT + `SO_INCOMING_CPU`（[13-01](01-scaling.md) accept-retry 范式）——分发、绑核、accept 全在内核路径完成，网关层零 master。
- **按 symbol 亲和**：BPF 选择器读 payload 首字段，把同一合约的行情永久固定给同一 worker——worker 内部 symbol 状态（订单簿）无锁、cache 常驻。
- **组播 fan-out**：默认每进程一份拷贝已够用；进程数 > 8 且单进程 CPU 成本可见时再考虑 UDS/共享内存分发。
- **滚动重启**：listener 迁移（5.14+）让升级策略进程不断线——对交易系统是实打实的可用性收益。

## 衔接

至此 chapter-13（零拷贝与高性能网络）收尾：页级零拷贝（收/发）、多核 steering、多监听分发三条线补完。下一篇 [chapter-14 TCP/UDP 内部机制](../chapter-14-tcp-udp-internals/README.md) 下沉到协议本身：TCP 状态机、定时器与拥塞控制在 v6.6 里的实现路径。

## 代码自测

<details>
<summary>Q1：SYN 到达 reuseport 组时，内核用哪个 hash 做分发？为什么必须保证 per-flow 一致？</summary>

`sk_reuseport_md.hash`——内核在 lookup 前算好的 4-tuple hash（siphash 系）。一致性是正确性要求：同一个客户端的 SYN 走 socket A，第三个 ACK 阶段的 lookup 也必须命中 A，否则连接请求散落在不同 socket 的队列里，accept 出来的是碎的。hash 输入是稳定 4-tuple，天然保证；BPF 程序如果自己选，也要保证同样输入同样输出（迁移场景除外，那是显式的重新选主）。
</details>

<details>
<summary>Q2：BPF 选择器返回了 num_socks（越界值），会发生什么？</summary>

静默退回 hash 分发：`reuseport_select_sock()` 里 `if (!sk2) sk2 = reuseport_select_sock_by_hash(...)`。BPF 的返回值经校验（bpf_run_sk_reuseport 内部），无效结果返回 NULL 而不是报错——数据路径永不因 BPF 逻辑错误而丢包，这是"增强不接管"的设计红线。但注意：退化为 hash 后 per-flow 亲和性可能与 BPF 时代不同，长连接已建立的不受影响（只影响新连接）。
</details>

<details>
<summary>Q3：worker 进程崩溃（listen socket 被 close），它 accept 队列里的半连接会怎样？</summary>

v5.14 之前：全灭——close 时 reqsk 队列被清空，客户端收到 RST，重连才进其他 worker。v5.14+ 且 attach 了支持迁移的 BPF 程序：`reuseport_migrate_sock()` 接管，reqsk 和已建连被"改嫁"给组内其他 listen socket（BPF 的 migrating_sk 非空分支可见原 socket）。没 attach BPF 时仍有内核默认迁移路径（by_hash 选新主）。生产滚动重启必须依赖这条路径。
</details>

<details>
<summary>Q4：reuseport_grow() 扩容时，正在并发执行的 reuseport_select_sock() 怎么办？</summary>

RCU 隔离。grow 分配新数组、复制内容、`rcu_assign_pointer` 发布新指针；并发中的 select 持有的是旧数组的 RCU 引用，访问完整安全，结束后自然释放。发布侧 `__reuseport_add_sock` 的 `smp_wmb()` 与消费侧 `smp_rmb()` 保证 `num_socks` 与 `socks[]` 内容的顺序可见性——选择者不会读到"数量已增大但内容未填充"的中间态。
</details>

<details>
<summary>Q5：为什么说"组播 + 多进程各自 bind"是免费 fan-out？代价上限在哪？</summary>

免费是因为网卡只收一份、分发在内核 lookup 里完成（每个 join 的 socket 一次 skb 拷贝），进程间完全隔离（一个 crash 不影响他人），零共享内存同步成本。代价是 O(N) 份 skb clone + N 次 UDP 入队——单条行情几十字节时 N 个进程成本可忽略；但 N > 8 且每秒百万级报文时，clone 的内存带宽和每进程的协议栈成本成为可见项，此时"单进程收 + 环形缓冲共享内存分发"（或 AF_XDP 直达）是规模化的下一步。
</details>
