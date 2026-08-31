# 04 — cgroup BPF：attach 语义、继承规则与返回码真相

> **来源：** LWN cgroup BPF 系列 + **v6.6 源码核对**（`kernel/bpf/cgroup.c`、`include/uapi/linux/bpf.h`、`net/core/filter.c`）
> **对应 Rosen：** 无
> **内核版本：** cgroup BPF 4.10+；SOCK_ADDR 4.18+；SOCKOPT 5.3+；全部结论基于 v6.6 核对

## 文档概述

本篇回答四个问题：

1. **cgroup BPF 到底能挂在哪、每类挂点的真实调用时机是什么**（注意：cgroup ingress 不是「包进协议栈时」，是「socket 入队时」——见 [01](01-ebpf-net-bootlin.md) 第 2.3 节）。
2. **`BPF_F_ALLOW_OVERRIDE` / `BPF_F_ALLOW_MULTI` / `BPF_F_REPLACE` 这些 flag 怎么决定子 cgroup 能不能挂、以及多个程序的执行顺序**（这是 cgroup BPF 最容易搞错的部分，本篇把 UAPI 注释原文贴出来逐行解释）。
3. **`BPF_CGROUP_SETSOCKOPT` 的返回码到底怎么用**——这块源码里的语义比文档复杂得多。
4. **`sock_ops` 的 16 个事件各自能拿到什么**，以及怎么用它在**不改应用代码**的前提下采到 RTT。

姊妹篇分工：

| 文件 | 本篇与它的关系 |
|------|---------------|
| [01-ebpf-net-bootlin.md](01-ebpf-net-bootlin.md) | 01 用源码钉死了 cgroup ingress/egress 的**真实调用点**；本篇展开 attach 规则 |
| [02-bpf.md](02-bpf.md) | 02 给出类型清单与 verifier 限制；本篇聚焦 cgroup 系列类型 |
| [03-xdp-bpf.md](03-xdp-bpf.md) | XDP 是「设备级」程序，与 cgroup 的「进程组级」形成对比 |

---

## 1. cgroup BPF 的本质：作用域从「设备」变成「进程组」

这是理解所有 cgroup BPF 的起点。

| 维度 | XDP / tc-BPF | cgroup BPF |
|------|-------------|-----------|
| 作用域 | **设备级**：挂到 `net_device`，该网卡上所有包都过 | **进程组级**：挂到 cgroup，只有该 cgroup 内进程的 socket 操作才触发 |
| 判定依据 | 包从哪个接口进来 | `sk->sk_cgrp_data`（socket 创建时从进程继承） |
| 粒度 | 每个包 | 每个 socket / 每个 socket 操作 |
| 典型用途 | 过滤、分流、转发 | 准入控制、地址重写、策略下发、可观测 |
| 性能定位 | **性能工具** | **治理工具**（见下方说明） |

**关键机制**：socket 在创建时从当前进程「继承」cgroup 归属，之后即使进程 `setns`/迁移，这个 socket 的 cgroup 也不变。所以 cgroup BPF 的规则对**已建立的 socket 是终身有效的**。

> **这是 cgroup BPF 最重要的工程性质**：你把交易进程挪到新 cgroup，它**已经建立的连接**仍然受旧 cgroup 的规则约束。要做热切换必须重建连接，或者把策略写成「新旧 cgroup 都挂一份」。

---

## 2. attach 类型全表（v6.6）

`enum bpf_attach_type`（`include/uapi/linux/bpf.h:994-1028`）里 cgroup 相关的成员：

| attach type | 程序类型 | 触发时机 | 上下文 |
|------------|---------|---------|--------|
| `BPF_CGROUP_INET_INGRESS` | `CGROUP_SKB` | **socket 入队时**（`sk_filter_trim_cap()`，`net/core/filter.c:138`） | `struct __sk_buff` |
| `BPF_CGROUP_INET_EGRESS` | `CGROUP_SKB` | **Netfilter POST_ROUTING 之后**（`ip_finish_output()`，`net/ipv4/ip_output.c:318`；组播 `ip_mc_finish_output():337`） | `struct __sk_buff` |
| `BPF_CGROUP_INET_SOCK_CREATE` | `CGROUP_SOCK` | `socket()` 创建时 | `struct bpf_sock` |
| `BPF_CGROUP_INET_SOCK_RELEASE` | `CGROUP_SOCK` | socket 释放时（6.9 才正式暴露，v6.6 枚举已存在） | `struct bpf_sock` |
| `BPF_CGROUP_SOCK_OPS` | `SOCK_OPS` | 16 种连接生命周期事件（见第 6 节） | `struct bpf_sock_ops` |
| `BPF_CGROUP_INET4_BIND` / `INET6_BIND` | `CGROUP_SOCK_ADDR` | `bind()` 时 | `struct bpf_sock_addr` |
| `BPF_CGROUP_INET4_POST_BIND` / `INET6_POST_BIND` | `CGROUP_SOCK_ADDR` | 内核绑定端口之后（程序可读取实际端口） | `struct bpf_sock_addr` |
| `BPF_CGROUP_INET4_CONNECT` / `INET6_CONNECT` | `CGROUP_SOCK_ADDR` | `connect()` 时（**可改 `user_ip4`/`user_port` 做重定向**） | `struct bpf_sock_addr` |
| `BPF_CGROUP_INET4_GETSOCKNAME` / `INET6_GETSOCKNAME` | `CGROUP_SOCK_ADDR` | `getsockname()` 时（可伪装本地地址） | `struct bpf_sock_addr` |
| `BPF_CGROUP_INET4_GETPEERNAME` / `INET6_GETPEERNAME` | `CGROUP_SOCK_ADDR` | `getpeername()` 时（可伪装对端地址） | `struct bpf_sock_addr` |
| `BPF_CGROUP_UDP4_SENDMSG` / `UDP6_SENDMSG` | `CGROUP_SOCK_ADDR` | UDP `sendmsg()` 时（可改目的地址） | `struct bpf_sock_addr` |
| `BPF_CGROUP_UDP4_RECVMSG` / `UDP6_RECVMSG` | `CGROUP_SOCK_ADDR` | UDP `recvmsg()` 时（**可改 `user_ip4` 影响 `msg_name` 回填**） | `struct bpf_sock_addr` |
| `BPF_CGROUP_SETSOCKOPT` | `CGROUP_SOCKOPT` | `setsockopt()` 时 | `struct bpf_sockopt` |
| `BPF_CGROUP_GETSOCKOPT` | `CGROUP_SOCKOPT` | `getsockopt()` 时 | `struct bpf_sockopt` |
| `BPF_CGROUP_SYSCTL` | `CGROUP_SYSCTL` | 读写 sysctl 时 | `struct bpf_sysctl` |

### 2.1 三类语义的区分

| 类别 | 成员 | 作用 |
|------|------|------|
| **数据路径**（每包） | `INET_INGRESS` / `INET_EGRESS` | 在 skb 上做过滤/记账；**只有这类是 per-packet 的** |
| **控制路径**（每 socket 操作） | `SOCK_CREATE`、`*_BIND`、`*_CONNECT`、`*_SENDMSG`、`SETSOCKOPT` 等 | 在 socket 系统调用时改写参数或拒绝 |
| **事件通知**（连接生命周期） | `SOCK_OPS`、`INET_SOCK_RELEASE` | 通知 + 可选改写，用于可观测和自动调优 |

**性能影响天差地别**：数据路径类的程序在**每个包**上跑；控制路径类只在系统调用时跑一次。做性能优化时，能放控制路径就别放数据路径。

---

## 3. ⭐ attach flags：继承与叠加的完整规则

这是 cgroup BPF 最容易被搞错的地方。UAPI 注释原文（`include/uapi/linux/bpf.h:1096-1118`）：

```c
 * A cgroup with MULTI or OVERRIDE flag allows any attach flags in
 * sub-cgroups.
 * A cgroup with NONE doesn't allow any programs in sub-cgroups.
 * Ex1:
 * cgrp1 (MULTI progs A, B) ->
 *    cgrp2 (OVERRIDE prog C) ->
 *      cgrp3 (MULTI prog D) ->
 *        cgrp4 (OVERRIDE prog E) ->
 *          cgrp5 (NONE prog F)
 * the event in cgrp5 triggers execution of F,D,A,B in that order.
 * if prog F is detached, the execution is E,D,A,B
 * if prog F and D are detached, the execution is E,A,B
 * if prog F, E and D are detached, the execution is C,A,B
 *
 * All eligible programs are executed regardless of return code from
 * earlier programs.
 */
#define BPF_F_ALLOW_OVERRIDE	(1U << 0)
#define BPF_F_ALLOW_MULTI	(1U << 1)
/* Generic attachment flags. */
#define BPF_F_REPLACE		(1U << 2)
#define BPF_F_BEFORE		(1U << 3)
#define BPF_F_AFTER		(1U << 4)
#define BPF_F_ID		(1U << 5)
```

### 3.1 逐条拆解

| flag | 值 | 含义 |
|------|-----|------|
| `BPF_F_ALLOW_OVERRIDE` | `1<<0` | 该 cgroup 允许**一个**程序；子 cgroup 可以继续挂（挂了就覆盖本层） |
| `BPF_F_ALLOW_MULTI` | `1<<1` | 该 cgroup 允许**多个**程序（按挂接顺序组成列表） |
| `BPF_F_REPLACE` | `1<<2` | 配合 `replace_bpf_fd`，替换列表里指定位置的旧程序 |
| `BPF_F_BEFORE` / `BPF_F_AFTER` | `1<<3` / `1<<4` | 配合 `expected_revision`/`relative_fd`，把新程序插入到指定程序的前/后 |
| `BPF_F_ID` | `1<<5` | 用 prog **ID** 而不是 fd 来指定位置 |
| 不指定任何 flag（NONE） | — | 允许**一个**程序，**且禁止子 cgroup 再挂** |

### 3.2 三条规则（把注释翻译成可执行的理解）

**规则 1：子 cgroup 的程序先执行，父 cgroup 的后执行。**
执行顺序是「从事件发生处（最内层 cgroup）向根遍历」。

**规则 2：`OVERRIDE` 层的语义是「继承链上只保留最内层的那个」。**
看注释给的链：`cgrp1(MULTI: A,B) → cgrp2(OVERRIDE: C) → cgrp3(MULTI: D) → cgrp4(OVERRIDE: E) → cgrp5(NONE: F)`

| cgrp5 的情况 | 执行顺序 | 解释 |
|-------------|---------|------|
| F 挂着 | **F, D, A, B** | F 在最内层；往上遇到 `cgrp4(OVERRIDE: E)`，E 被 F 覆盖；再往上 `cgrp3(MULTI: D)` 全部执行；再往上 `cgrp2(OVERRIDE: C)`，C 被 D 覆盖；`cgrp1(MULTI: A,B)` 全部执行 |
| F 摘掉 | **E, D, A, B** | E 变成最内层，不再被覆盖 |
| F、D 都摘掉 | **E, A, B** | D 没了，C 不再被 D 覆盖？**注意：实际是 E, A, B，C 仍然被 E 覆盖**——因为 E 在更内层 |
| F、E、D 都摘掉 | **C, A, B** | C 终于生效 |

**这张表的推理要点**：一个 `OVERRIDE` 层的程序**只有在它和根之间没有其他程序时**才会执行。换句话说，`OVERRIDE` 是「这一层最多一个，且被更内层的程序顶掉」。

**规则 3（最容易忽略）：`All eligible programs are executed regardless of return code from earlier programs.`**

**所有符合条件的程序都会被执行，前面程序的返回值不会中断链条。** 这和 Netfilter（`NF_ACCEPT`/`NF_DROP` 会短路）以及 tc（`TC_ACT_*` 会决定后续）**完全不同**。

> **工程含义**：cgroup BPF 里**不能靠返回码做短路优化**。如果你在父 cgroup 挂了一个「放行白名单」程序，它返回 1 之后，子 cgroup 的「拒绝黑名单」程序**照样会跑**。要做「先白后黑」的语义，必须在程序之间共享 map 状态，或者干脆合并成一个程序。

### 3.3 实操命令

```bash
# 挂载（OVERRIDE：允许子 cgroup 覆盖，本层只能挂一个）
bpftool cgroup attach /sys/fs/cgroup/trading/ ingress id <PROG_ID>

# 挂载（MULTI：本层可挂多个，按顺序执行）
bpftool cgroup attach /sys/fs/cgroup/trading/ ingress_multi id <PROG_ID1>
bpftool cgroup attach /sys/fs/cgroup/trading/ ingress_multi id <PROG_ID2>

# 查看某个 cgroup 上挂了什么
bpftool cgroup show /sys/fs/cgroup/trading/

# 查看详细（含 attach flags、程序名、是否有效）
bpftool cgroup show /sys/fs/cgroup/trading/ --pretty

# 摘掉
bpftool cgroup detach /sys/fs/cgroup/trading/ ingress id <PROG_ID>

# 所有 attach type 的名字
bpftool cgroup help
```

**`bpftool cgroup show` 的输出里有一列 `attach_flags`**，直接显示 `multi` / `override`——这是排查「为什么我的程序没被执行」的第一步。

---

## 4. ⭐ `BPF_CGROUP_SETSOCKOPT` 的返回码真相

这块源码里的语义比大多数文档复杂。看 `kernel/bpf/cgroup.c` 的 `__cgroup_bpf_run_filter_setsockopt()`（v6.6）：

```c
	/* Allocate a bit more than the initial user buffer for
	 * BPF program. The canonical use case is overriding
	 * TCP_CONGESTION(nv) to TCP_CONGESTION(cubic).
	 */
	max_optlen = max_t(int, 16, *optlen);
	max_optlen = sockopt_alloc_buf(&ctx, max_optlen, &buf);
	if (max_optlen < 0)
		return max_optlen;

	ctx.optlen = *optlen;

	if (copy_from_user(ctx.optval, optval, min(*optlen, max_optlen)) != 0) {
		ret = -EFAULT;
		goto out;
	}

	lock_sock(sk);
	ret = bpf_prog_run_array_cg(&cgrp->bpf, CGROUP_SETSOCKOPT,
				    &ctx, bpf_prog_run, 0, NULL);
	release_sock(sk);

	if (ret)
		goto out;                      /* ← ① */

	if (ctx.optlen == -1) {
		/* optlen set to -1, bypass kernel */
		ret = 1;                       /* ← ② */
	} else if (ctx.optlen > max_optlen || ctx.optlen < -1) {
		/* optlen is out of bounds */
		if (*optlen > PAGE_SIZE && ctx.optlen >= 0) {
			pr_info_once("bpf setsockopt: ignoring program buffer with optlen=%d (max_optlen=%d)\n",
				     ctx.optlen, max_optlen);
			ret = 0;
			goto out;
		}
		ret = -EFAULT;                 /* ← ③ */
	} else {
		/* optlen within bounds, run kernel handler */
		ret = 0;                       /* ← ④ */

		/* export any potential modifications */
		*level = ctx.level;
		*optname = ctx.optname;

		/* optlen == 0 from BPF indicates that we should
		 * use original userspace data.
		 */
		if (ctx.optlen != 0) {
			*optlen = ctx.optlen;
			...
			if (!sockopt_buf_allocated(&ctx, &buf)) {
				void *p = kmalloc(ctx.optlen, GFP_USER);

				if (!p) {
					ret = -ENOMEM;
```

### 4.1 四条语义（对照上面标号）

| 标号 | condition | 结果 |
|------|-----------|------|
| ① | **BPF 程序返回非 0** | 该返回值**直接成为 `setsockopt()` 的返回值**；链上剩余的 BPF 程序**和**内核处理**全部跳过** |
| ② | BPF 返回 0 **且** 设了 `ctx->optlen = -1` | **BPF 已完全接管**，跳过内核 handler，返回 1 |
| ③ | BPF 返回 0，但 `optlen` 越界（`< -1` 或 `> max_optlen`） | 若原 `optlen > PAGE_SIZE` 且新 `optlen >= 0` → 打印一次 `pr_info_once` 警告并**照旧继续**（`ret = 0`）；否则 `-EFAULT` |
| ④ | BPF 返回 0，`optlen` 合法 | **调内核 handler**。若 `optlen == 0` 用原始用户态数据；否则用 BPF 填的 buffer（可能需要 `kmalloc` 搬运，可 `-ENOMEM`） |

### 4.2 三个必须知道的细节

1. **内核为每个 `setsockopt` 预留至少 16 字节的 buffer**（`max_t(int, 16, *optlen)`），注释里说明这是为了「把 `TCP_CONGESTION` 从 `nv` 覆盖成 `cubic`」——后者的名字更长。**所以你在 BPF 里能安全写入的 optval 长度至少是 16 字节**，超过原 `optlen` 的部分要走 `max_optlen` 判定。
2. **整个 BPF 执行期间持有 `lock_sock(sk)`**。这意味着：① 你的程序**不能睡眠**（BPF 本来也不行）；② **如果同一 socket 上有大量并发 setsockopt，它们会在 `lock_sock` 上串行化**——这是个容易被忽略的锁竞争点。
3. **`GETSOCKOPT` 的短路条件不同**：同样是 `kernel/bpf/cgroup.c`，GETSOCKOPT 分支写的是 `if (ret < 0) goto out;`，**只有负返回值才短路**；0 或正数会继续走内核 handler 并 `copy_to_user`。这与 SETSOCKOPT 的 `if (ret)`（任何非 0 都短路）**不对称**。写代码时不要想当然。

### 4.3 上下文结构与可用字段

```c
/* include/uapi/linux/bpf.h:7159 */
struct bpf_sockopt {
	__bpf_md_ptr(struct bpf_sock *, sk);
	__bpf_md_ptr(void *, optval);
	__bpf_md_ptr(void *, optval_end);

	__s32	level;
	__s32	optname;
	__s32	optlen;
	__s32	retval;
};
```

---

## 5. `BPF_CGROUP_SOCK_ADDR`：能做和不能做的事

`struct bpf_sock_addr`（`include/uapi/linux/bpf.h:2700` 起）可分两类字段：

| 类别 | 字段 | 可写？ |
|------|------|--------|
| **内核看到的地址**（内核实际要连/绑的地址） | `user_ip4`、`user_ip6`、`user_port` | — |
| **用户看到的地址**（回填给 `msg_name` / `sockaddr`） | `msg_src_ip4`、`msg_src_ip6`、`msg_src_port` | — |

**各 attach 点的可写性（v6.6）**：

| attach type | 可改什么 | 典型用途 |
|------------|---------|---------|
| `INET4/6_CONNECT` | `user_ip4/6`、`user_port`；`msg_src_ip4/6`、`msg_src_port` | **强制走本地代理**、透明劫持连接 |
| `INET4/6_BIND` | `user_ip4/6`、`user_port` | 强制绑定地址/端口 |
| `INET4/6_POST_BIND` | **只读**（内核已绑定完） | 记录实际分配到的端口 |
| `UDP4/6_SENDMSG` | `user_ip4/6`、`user_port` | 改写 UDP 目的地址（做透明代理） |
| `UDP4/6_RECVMSG` | `msg_src_ip4/6`、`msg_src_port` | **改写回填给应用的源地址** |
| `INET4/6_GETSOCKNAME` | `msg_src_ip4/6`、`msg_src_port` | 伪装 `getsockname()` 的结果 |
| `INET4/6_GETPEERNAME` | `msg_src_ip4/6`、`msg_src_port` | 伪装 `getpeername()` 的结果 |

**返回码**：`0` = 拒绝该操作（`connect()`/`bind()` 返回 `-EPERM`）；`1` = 放行。

> **⚠️ 常见的坑**：在 `UDP4_RECVMSG` 里，你改的是 **`msg_src_*`**（用户看到的源地址），**不是** `user_*`（内核实际用的）。改错了没报错，只是没效果。

---

## 6. `sock_ops`：不改应用代码采集 TCP 指标的唯一手段

### 6.1 16 个事件（`include/uapi/linux/bpf.h:6727-6800`）

| 事件 | 触发时机 | 返回值语义 | HFT 用途 |
|------|---------|-----------|---------|
| `BPF_SOCK_OPS_VOID` | 占位 | — | — |
| `BPF_SOCK_OPS_TIMEOUT_INIT` | 建连时 | 返回 SYN-RTO 值；`-1` = 用默认 | **为跨机房链路定制初始 RTO** |
| `BPF_SOCK_OPS_RWND_INIT` | 建连时 | 返回初始通告窗口（包数）；`-1` = 默认 | 小包高频场景调窗口 |
| `BPF_SOCK_OPS_TCP_CONNECT_CB` | 主动建连前 | — | 记录建连 |
| `BPF_SOCK_OPS_ACTIVE_ESTABLISHED_CB` | 主动建连完成 | — | 记录连接建立时刻 |
| `BPF_SOCK_OPS_PASSIVE_ESTABLISHED_CB` | 被动建连完成 | — | 同上 |
| `BPF_SOCK_OPS_NEEDS_ECN` | 拥塞控制需 ECN 时 | `1` = 启用 ECN，`0` = 不启用 | **行情组播/单播链路的 ECN 策略** |
| `BPF_SOCK_OPS_BASE_RTT` | 拥塞控制查询 base RTT | 返回 base RTT | **为 BBR 等算法喂正确的 base RTT** |
| `BPF_SOCK_OPS_RTO_CB` | **RTO 触发时**<br>arg1=`icsk_retransmits`, arg2=`icsk_rto`, arg3=是否超时 | — | ⭐ **重传告警** |
| `BPF_SOCK_OPS_RETRANS_CB` | **skb 重传时**<br>arg1=首字节 seq, arg2=段数, arg3=`tcp_transmit_skb` 返回值 | — | ⭐ **重传计数** |
| `BPF_SOCK_OPS_STATE_CB` | **TCP 状态变化时**<br>arg1=old_state, arg2=new_state | — | ⭐ **连接状态机追踪** |
| `BPF_SOCK_OPS_TCP_LISTEN_CB` | `listen()` 后进入 LISTEN | — | 监听点监控 |
| `BPF_SOCK_OPS_RTT_CB` | **每个 RTT** | — | ⭐⭐ **RTT 采样** |
| `BPF_SOCK_OPS_PARSE_HDR_OPT_CB` | 解析 TCP 选项 | — | 自定义 TCP 选项 |
| `BPF_SOCK_OPS_HDR_OPT_LEN_CB` | 预留 TCP 选项空间 | 返回需要的字节数 | 同上 |
| `BPF_SOCK_OPS_WRITE_HDR_OPT_CB` | 写入 TCP 选项 | — | 同上 |

### 6.2 ⭐ 回调必须先「订阅」：否则部分事件不会触发

`RTO_CB` / `RETRANS_CB` / `STATE_CB` / `RTT_CB` 这几个**高频**事件默认是不触发的，必须在程序里显式设置 `bpf_sock_ops_cb_flags`（`include/uapi/linux/bpf.h:6673-6704`）：

```c
#define BPF_SOCK_OPS_RTO_CB_FLAG	(1<<0)
#define BPF_SOCK_OPS_RETRANS_CB_FLAG	(1<<1)
#define BPF_SOCK_OPS_STATE_CB_FLAG	(1<<2)
#define BPF_SOCK_OPS_RTT_CB_FLAG	(1<<3)
#define BPF_SOCK_OPS_PARSE_ALL_HDR_OPT_CB_FLAG		(1<<4)
#define BPF_SOCK_OPS_PARSE_UNKNOWN_HDR_OPT_CB_FLAG	(1<<5)
```

写法（通常在 `ACTIVE_ESTABLISHED_CB` / `PASSIVE_ESTABLISHED_CB` 里设一次）：

```c
SEC("sockops")
int sockops_prog(struct bpf_sock_ops *ops)
{
	switch (ops->op) {
	case BPF_SOCK_OPS_ACTIVE_ESTABLISHED_CB:
	case BPF_SOCK_OPS_PASSIVE_ESTABLISHED_CB:
		/* 订阅 RTT + 重传 + 状态变化回调 */
		bpf_sock_ops_cb_flags_set(ops,
			BPF_SOCK_OPS_RTT_CB_FLAG     |
			BPF_SOCK_OPS_RETRANS_CB_FLAG |
			BPF_SOCK_OPS_STATE_CB_FLAG);
		break;

	case BPF_SOCK_OPS_RTT_CB:
		/* ops->srtt_us 是平滑 RTT（微秒），ops->rtt_min 是最小 RTT */
		record_rtt(ops);
		break;

	case BPF_SOCK_OPS_RETRANS_CB:
		/* ops->args[0] = seq, args[1] = segments */
		count_retrans(ops);
		break;
	}
	return 0;
}
```

> **这是 cgroup BPF 最常见的「程序挂上了但什么都没发生」的原因**：没设 `bpf_sock_ops_cb_flags_set()`，`RTT_CB` 就永远不会触发。

### 6.3 `struct bpf_sock_ops` 的关键字段

| 字段 | 含义 |
|------|------|
| `op` | 当前事件类型（就是上表的枚举值） |
| `args[4]` | 事件参数（含义见上表） |
| `family`、`remote_ip4/6`、`local_ip4/6` | 地址 |
| `remote_port`、`local_port` | 端口（**注意是主机字节序**） |
| `srtt_us` | **平滑 RTT（微秒）** |
| `rtt_min`、`rtt_var_us` | 最小 RTT、RTT 方差 |
| `snd_cwnd`、`snd_ssthresh` | 拥塞窗口、慢启动阈值 |
| `state` | TCP 状态 |
| `is_fullsock` | 是否为 full socket（不是时很多字段不可用） |
| `bpf_sock_ops_cb_flags` | 回调订阅开关 |

> **HFT 价值**：`srtt_us` + `rtt_min` + `snd_cwnd` 这三个字段组合起来，就等价于**在内核里跑了一个零侵入的 TCP 健康度探针**——不需要在交易程序里加任何埋点，也不需要 `ss -i` 轮询（轮询有采样间隔，会漏掉瞬时抖动）。

---

## 7. 与 XDP / tc 的三方对比

| 维度 | XDP-BPF | tc-BPF | cgroup-BPF |
|------|---------|--------|-----------|
| 作用域 | 设备级 | 设备级 | **进程组级** |
| 判定归属 | 包从哪来 | 包从哪来 | `sk->sk_cgrp_data`（socket 创建时继承，**终身不变**） |
| 时机（RX） | 驱动层，**无 skb** | `__netif_receive_skb_core`，有 skb | **`sk_filter_trim_cap()`，socket 入队时** |
| 时机（TX） | 无 | `__dev_queue_xmit`，qdisc 前 | **`ip_finish_output()`，POST_ROUTING 之后** |
| 粒度 | 每个包 | 每个 skb | 每包（仅 INET_INGRESS/EGRESS）或每 socket 操作 |
| 返回码短路 | 是（`XDP_DROP` 等决定命运） | 是（`TC_ACT_*`） | **否——「All eligible programs are executed regardless of return code」** |
| 性能定位 | 消除开销 | 分类/整形 | **治理/可观测** |
| 是否能改 skb 元数据 | ❌（无 skb） | ✅ | 部分（`__sk_buff` 的只读/可写子集） |

---

## 8. 观测与排障

```bash
# ① 该 cgroup 上挂了什么、attach flags 是什么
bpftool cgroup show /sys/fs/cgroup/trading/ --pretty

# ② 程序是否被执行
sysctl -w kernel.bpf_stats_enabled=1
bpftool prog show id <ID>            # run_cnt / run_time_ns
sysctl -w kernel.bpf_stats_enabled=0

# ③ cgroup 相关的 tracepoint
bpftrace -l 'tracepoint:cgroup:*'
bpftrace -e 'tracepoint:cgroup:cgroup_attach_task { printf("%s -> %s\n", str(args->dst_path), str(args->comm)); }'

# ④ 被 cgroup BPF 丢的包（有 drop reason）
bpftrace -e 'tracepoint:skb:kfree_skb { @[args->reason] = count(); }'
#   相关 reason：SKB_DROP_REASON_BPF_CGROUP_EGRESS

# ⑤ sock_ops 是否触发
#   在程序里维护一个 PERCPU_ARRAY 计数器，用 bpftool map dump 读
bpftool map dump name sockops_stats
```

**常见故障表：**

| 现象 | 根因 | 排查 |
|------|------|------|
| 程序挂上了但从未执行 | cgroup ingress/egress 只对**有 socket 的包**生效；转发包、未加入的组播包不走 | 改用 tc/XDP，或用 `tracepoint:skb:kfree_skb` 确认 |
| `sock_ops` 的 `RTT_CB` 不触发 | 没调 `bpf_sock_ops_cb_flags_set()` 订阅 | 在 `*_ESTABLISHED_CB` 里设 flag |
| 子 cgroup 挂不上去 | 父 cgroup 挂的是 NONE（无 flag） | `bpftool cgroup show` 看 `attach_flags`；父层改用 `multi`/`override` |
| 期望「白名单优先」但黑名单仍然生效 | **cgroup BPF 不停短路**：所有程序都跑 | 用 map 在程序间共享决策，或合并成一个程序 |
| `setsockopt()` 被 BPF 拦截后返回值不对 | SETSOCKOPT 的 `if (ret)` 会把 BPF 的非 0 返回值直接当系统调用返回值；GETSOCKOPT 只有负数才短路 | 检查返回码；见第 4 节 |
| 高并发下 `setsockopt` 变慢 | BPF 执行期间持有 `lock_sock(sk)`，串行化 | 减少 setsockopt 频率，或把逻辑挪到 `SOCK_OPS` |
| 进程迁移到新 cgroup 后策略不变 | socket 的 cgroup 归属在**创建时确定，终身不变** | 必须重建连接，或新旧 cgroup 都挂 |

---

## 9. HFT 要点

1. **cgroup BPF 是治理工具，不是性能工具。** 它唯一 per-packet 的两个挂载点（INGRESS/EGRESS）分别位于 socket 入队和 POST_ROUTING 之后——**包走到那里时该付的 CPU 已经全部付完了**。想省 CPU 只能上 XDP 或 tc。
2. **它的真实价值是「零侵入可观测」**：`sock_ops` 的 `srtt_us` / `rtt_min` / `snd_cwnd` / `RETRANS_CB` / `STATE_CB` 让你在**不碰交易程序一行代码**的前提下拿到 TCP 层的健康度数据。这比在应用里埋点安全得多（不会引入延迟抖动，不会改发布流程）。
3. **进程隔离**：交易进程和行情进程分属不同 cgroup，各自挂独立的网络策略。配合 `BPF_CGROUP_INET4_CONNECT` 可以强制交易进程只能连指定的行情网关/交易所前置地址，防止配置错误连错环境。
4. **`BASE_RTT` 和 `RWND_INIT` 是可写的**，这给了你在内核里针对特定链路调优 TCP 的能力——比如为跨机房的组播补包通道（TCP）设置一个更贴合实际的 base RTT，避免 BBR 在慢启动阶段过度探测。
5. **永远记住「不停短路」这条规则**：多个 cgroup 程序叠加时，前一个的返回值不影响后一个是否执行。要表达「优先级」必须靠共享 map。
6. **`setsockopt` 路径上有 `lock_sock()`**。如果你的程序在跑高频 `setsockopt`（比如频繁调 `TCP_NODELAY` 或 `SO_RCVBUF`），cgroup SOCKOPT 程序会把它串行化。这类调用在初始化时一次性做完。
7. **socket 的 cgroup 归属终身不变**——做灰度切换时这是个陷阱，也是个保障（长连接策略稳定）。

---

## 10. 与 Rosen 3.x 的差异

Rosen 3.x 的时代（2.6/3.x 内核）**完全没有 cgroup BPF**，只有：

| Rosen 时代的能力 | 现在的对应物 |
|-----------------|-------------|
| `SO_BINDTODEVICE`（绑定到设备） | `BPF_CGROUP_INET4_BIND` 可改 `user_ip4` |
| iptables `-m owner`（按 uid/gid/pid 匹配） | cgroup BPF（按 cgroup 匹配，**粒度更细、可编程**） |
| `tcp_(diag)` / `ss -i` 轮询 | `sock_ops` 事件驱动（无采样间隔） |
| SELinux/AppArmor 网络规则 | `BPF_PROG_TYPE_LSM` + cgroup BPF |

cgroup BPF 把「按进程做网络策略」从**固定属性的匹配**（uid/gid）升级成了**可编程的判定**（任意 BPF 逻辑 + map 状态）。

---

## 11. 代码自测

<details>
<summary>Q1：你在父 cgroup 挂了一个「放行白名单」程序（命中白名单返回 1），在子 cgroup 挂了一个「拒绝黑名单」程序（命中黑名单返回 0）。你期望白名单先短路，结果一条本该放行的连接被拒了。为什么？</summary>

**因为 cgroup BPF 不做短路。** UAPI 注释（`include/uapi/linux/bpf.h:1117-1118`）写得非常直白：

```
 * All eligible programs are executed regardless of return code from
 * earlier programs.
```

**所有符合条件的程序都会被执行，与前面程序返回什么都无关。**

这和另外两个 hook 形成鲜明对比：

| hook | 返回码是否中断链条 |
|------|------------------|
| Netfilter | ✅ `NF_DROP` / `NF_ACCEPT` 决定后续 |
| tc（clsact） | ✅ `TC_ACT_SHOT` / `TC_ACT_OK` / `TC_ACT_STOLEN` 决定后续 |
| **cgroup BPF** | ❌ **不停短路，全部执行** |

**所以执行顺序是：**

```
事件发生在子 cgroup
  → 子 cgroup 的「拒绝黑名单」程序先跑（子层先于父层）
     → 命中黑名单 → 返回 0（拒绝）
  → 父 cgroup 的「放行白名单」程序后跑
     → 命中白名单 → 返回 1（放行）
  → 内核汇总：只要有一个返回 0，整个操作被拒绝
```

**白名单程序跑得比黑名单晚，且它的「放行」无法撤销黑名单已经做出的「拒绝」。**

**三种修法：**

1. **合并成一个程序**，挂在同一个 cgroup 上，内部按优先级判断（白名单优先，命中就 return 1）。**最简单，推荐。**
2. **用共享 map 传递决策**：白名单程序先执行（挂到更内层 cgroup），命中时往 map 里写 `allowed=1`；黑名单程序查这个 map，若 `allowed` 则跳过。注意执行顺序是**子层先、父层后**，所以要让白名单挂到**更内层**。
3. **只挂一层，用 map 存两套规则**，在程序里统一裁决。

**顺带复习执行顺序规则**（`include/uapi/linux/bpf.h:1104-1116` 的 Ex1）：

```
cgrp1 (MULTI progs A, B) ->
   cgrp2 (OVERRIDE prog C) ->
     cgrp3 (MULTI prog D) ->
       cgrp4 (OVERRIDE prog E) ->
         cgrp5 (NONE prog F)
the event in cgrp5 triggers execution of F,D,A,B in that order.
```

- **子层先、父层后**（F 最内层，最先跑）。
- `OVERRIDE` 层（C、E）的程序**会被更内层的程序顶掉**：F 存在时 E 不跑；F 摘掉后 E 才跑。
- `MULTI` 层（A、B、D）的程序**全部都跑**。

</details>

<details>
<summary>Q2：你写了一个 <code>sock_ops</code> 程序想采集 RTT，程序挂上了、<code>bpftool prog show</code> 里 <code>run_cnt</code> 也在涨，但 <code>RTT_CB</code> 分支里的计数器一直是 0。为什么？</summary>

**因为你没有订阅 RTT 回调。**

`RTT_CB`、`RTO_CB`、`RETRANS_CB`、`STATE_CB` 这几个**高频**事件在 v6.6 默认是关闭的，必须通过 `bpf_sock_ops_cb_flags_set()` 显式打开（`include/uapi/linux/bpf.h:6673-6704`）：

```c
#define BPF_SOCK_OPS_RTO_CB_FLAG	(1<<0)
#define BPF_SOCK_OPS_RETRANS_CB_FLAG	(1<<1)
#define BPF_SOCK_OPS_STATE_CB_FLAG	(1<<2)
#define BPF_SOCK_OPS_RTT_CB_FLAG	(1<<3)
#define BPF_SOCK_OPS_PARSE_ALL_HDR_OPT_CB_FLAG		(1<<4)
#define BPF_SOCK_OPS_PARSE_UNKNOWN_HDR_OPT_CB_FLAG	(1<<5)
```

**为什么默认关？** 因为这几个事件在**每个 RTT / 每次重传 / 每次状态变化**时触发，属于热路径。内核的设计是「你不订阅就不付代价」——`run_cnt` 涨是因为 `ACTIVE_ESTABLISHED_CB` 这类低频事件在跑，说明程序确实挂上了、也确实在被调用，只是 RTT 事件没订阅。

**修法**：在连接建立时订阅一次：

```c
SEC("sockops")
int sockops_prog(struct bpf_sock_ops *ops)
{
	switch (ops->op) {
	case BPF_SOCK_OPS_ACTIVE_ESTABLISHED_CB:
	case BPF_SOCK_OPS_PASSIVE_ESTABLISHED_CB:
		bpf_sock_ops_cb_flags_set(ops,
			BPF_SOCK_OPS_RTT_CB_FLAG     |   /* 每 RTT 触发 */
			BPF_SOCK_OPS_RETRANS_CB_FLAG |   /* 每次重传触发 */
			BPF_SOCK_OPS_STATE_CB_FLAG);     /* 状态变化触发 */
		break;

	case BPF_SOCK_OPS_RTT_CB:
		/* ops->srtt_us  : 平滑 RTT（微秒）
		 * ops->rtt_min  : 观测到的最小 RTT
		 * ops->rtt_var_us: RTT 方差
		 */
		record(ops->srtt_us, ops->rtt_min);
		break;
	}
	return 0;
}
```

**两个配套的坑：**

1. **订阅是 per-socket 的**，不是全局的。所以必须在**每个连接**建立时都设一次（这就是上面放到 `*_ESTABLISHED_CB` 里的原因）。
2. **`ops->local_port` / `remote_port` 是主机字节序**，`local_ip4` / `remote_ip4` 是网络字节序。做 key 的时候别搞混，否则 map 查不到。

**验证订阅是否生效**：读 `ops->bpf_sock_ops_cb_flags` 并打到 map 里，或者用 `bpftool map dump` 看 `*_ESTABLISHED_CB` 分支的计数是否在涨。

</details>

<details>
<summary>Q3：你在 <code>BPF_CGROUP_SETSOCKOPT</code> 程序里想覆盖 <code>TCP_CONGESTION</code>，返回了 1 表示「我处理完了」，结果发现内核的 setsockopt 仍然被调用了；而另一个场景里你返回了 1，用户的 <code>setsockopt()</code> 却收到了返回值 1（看起来像成功但没有实际生效）。这两次分别错在哪？</summary>

**两次都错在对返回码语义的理解上。** 看 `kernel/bpf/cgroup.c` 的源码：

```c
	lock_sock(sk);
	ret = bpf_prog_run_array_cg(&cgrp->bpf, CGROUP_SETSOCKOPT,
				    &ctx, bpf_prog_run, 0, NULL);
	release_sock(sk);

	if (ret)
		goto out;                       /* ← ① 非 0 就直接短路 */

	if (ctx.optlen == -1) {
		/* optlen set to -1, bypass kernel */
		ret = 1;                        /* ← ② 这才是「绕过内核」 */
	} else if (ctx.optlen > max_optlen || ctx.optlen < -1) {
		...
	} else {
		/* optlen within bounds, run kernel handler */
		ret = 0;                        /* ← ④ 默认：继续调内核 */
	}
```

**完整语义表：**

| BPF 返回值 | `ctx->optlen` | 结果 |
|-----------|--------------|------|
| **非 0** | 任意 | **该值直接成为 `setsockopt()` 的返回值**；链上剩余 BPF 程序 + 内核 handler **全部跳过** |
| 0 | **`-1`** | BPF 已完全接管，跳过内核 handler，返回 1 |
| 0 | `0` | 用**原始用户态数据**继续调内核 handler |
| 0 | 合法（`1..max_optlen`） | 用 **BPF 填的 buffer** 继续调内核 handler |
| 0 | 越界 | `*optlen > PAGE_SIZE && optlen >= 0` → `pr_info_once` 警告 + 照旧继续；否则 `-EFAULT` |

**对照你的两个场景：**

- **场景一（返回 1 但内核仍被调用）**：你把「返回 1」理解成了「继续让内核处理」。实际上返回非 0 是 **短路**（①），内核 handler 应该**不会**被调用。如果你观察到内核仍被调用，说明**你返回的其实是 0**——常见原因是程序走了另一条分支（`level`/`optname` 不匹配时 `return 0` 提前返回了）。请在每个分支验证返回值。
- **场景二（返回 1，用户拿到返回值 1 但没生效）**：这正是 ① 的行为——你的返回值 1 被直接当作 `setsockopt()` 的返回值交给了用户，而**内核 handler 被完全跳过**，所以拥塞算法其实没被设置。用户看到返回值 1 会当成「成功」（虽然 `setsockopt` 约定成功是 0），于是静默失败。

**正确写法（覆盖 TCP_CONGESTION）：**

```c
SEC("cgroup/setsockopt")
int setsockopt_prog(struct bpf_sockopt *ctx)
{
	if (ctx->level != SOL_TCP || ctx->optname != TCP_CONGESTION)
		return 1;      /* 不关心的选项：返回值语义见下 */

	/* 检查 optval 空间是否够写 "cubic" */
	if (ctx->optval_end - ctx->optval < 6)
		return 1;

	__builtin_memcpy(ctx->optval, "cubic", 6);
	ctx->optlen = 6;       /* 告诉内核用 BPF 的 buffer，长度 6 */

	return 0;              /* ⭐ 返回 0 = 继续，内核会用 BPF 填的 optval */
}
```

**如果确实要「完全接管、不让内核碰」**，则：

```c
	__builtin_memcpy(ctx->optval, "cubic", 6);
	ctx->optlen = -1;      /* ⭐ -1 = bypass kernel */
	return 0;              /* ⭐ 必须返回 0，否则走 ① 短路分支 */
```

**三个附带细节：**

1. **内核为每个 setsockopt 预留至少 16 字节 buffer**：`max_optlen = max_t(int, 16, *optlen)`，注释明确说是为了「把 `TCP_CONGESTION` 从 `nv` 覆盖成 `cubic`」这类场景（新名字更长）。所以写 6 字节的 `cubic` 是安全的。
2. **GETSOCKOPT 的短路条件不同**：源码里是 `if (ret < 0) goto out;`，**只有负数才短路**，0 和正数都会继续走内核 handler 并 `copy_to_user`。**两个方向不对称，别想当然。**
3. **整个 BPF 执行期间持有 `lock_sock(sk)`**。高频 `setsockopt` 会被串行化，且有 `kmalloc(GFP_USER)` 的可能（当 BPF 填的 optlen 需要交给内核 handler 时）。

</details>

---

## 导航

- **本篇：** [01-ebpf-net-bootlin.md](01-ebpf-net-bootlin.md) · [02-bpf.md](02-bpf.md) · [03-xdp-bpf.md](03-xdp-bpf.md)
- **相关：** [chapter-09-tc-bpf](../../chapter-09-tc-bpf/) tc-BPF · [chapter-05-xdp-architecture](../../chapter-05-xdp-architecture/) XDP · `06.7-bpf-observability/` 可观测性
- **章节主页：** [README](../README.md)
