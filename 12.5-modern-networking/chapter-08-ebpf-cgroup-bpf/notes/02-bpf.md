# 02 — BPF 类型系统、map 与 verifier 的真实约束

> **来源：** Linux 内核文档 `Documentation/bpf/` + **v6.6 源码逐条核对**
> **对应 Rosen：** 无
> **本篇立场：** 网络上流传的「BPF 限制清单」有大量过时或错误的条目（尤其是指令数上限）。本篇所有数字都来自 v6.6 源码，并明确标注**哪些文档注释已经与代码不一致**。

## 文档概述

本篇回答三个问题：

1. **v6.6 里到底有多少种程序类型、多少种 map 类型，各自是干什么的。**
2. **谁有资格把一个 BPF 程序塞进内核**（权限模型，这块几乎没人写清楚）。
3. **verifier 真正卡住你的是什么**——不是「不能有循环」这种口号，而是具体的限流常量，以及**哪些常量的文档注释已经过时**。

姊妹篇分工：

| 文件 | 本篇与它的关系 |
|------|---------------|
| [01-ebpf-net-bootlin.md](01-ebpf-net-bootlin.md) | 01 给出「类型 × 挂载点 × 工具」横表；本篇给出类型的**全部清单**和**能力边界** |
| [03-xdp-bpf.md](03-xdp-bpf.md) | 本篇的 verifier 规则在 XDP 上的具体体现（包边界检查）由 03 展开 |
| [04-cgroup-bpf.md](04-cgroup-bpf.md) | 本篇的 cgroup 类型在 04 展开 attach 语义 |

---

## 1. 程序类型：v6.6 共 33 种

`enum bpf_prog_type`（`include/uapi/linux/bpf.h:958-990`），从 `BPF_PROG_TYPE_UNSPEC` 到 `BPF_PROG_TYPE_NETFILTER`，**共 33 个成员**。

完整清单（按内核版本引入顺序大致排列）：

| # | 类型 | 引入 | 领域 | 上下文结构体 |
|---|------|------|------|-------------|
| 0 | `UNSPEC` | — | 占位 | — |
| 1 | `SOCKET_FILTER` | 3.19 | 网络 | `struct __sk_buff` |
| 2 | `KPROBE` | 4.1 | 追踪 | `struct pt_regs` |
| 3 | `SCHED_CLS` | 4.1 | 网络（tc） | `struct __sk_buff` |
| 4 | `SCHED_ACT` | 4.1 | 网络（tc action） | `struct __sk_buff` |
| 5 | `TRACEPOINT` | 4.7 | 追踪 | tracepoint 参数结构 |
| 6 | `XDP` | 4.8 | 网络 | `struct xdp_md` |
| 7 | `PERF_EVENT` | 4.9 | 追踪 | `struct bpf_perf_event_data` |
| 8 | `CGROUP_SKB` | 4.10 | cgroup 网络 | `struct __sk_buff` |
| 9 | `CGROUP_SOCK` | 4.10 | cgroup 网络 | `struct bpf_sock` |
| 10 | `LWT_IN` | 4.10 | 路由隧道 | `struct __sk_buff` |
| 11 | `LWT_OUT` | 4.10 | 路由隧道 | `struct __sk_buff` |
| 12 | `LWT_XMIT` | 4.12 | 路由隧道 | `struct __sk_buff` |
| 13 | `SOCK_OPS` | 4.13 | cgroup 网络 | `struct bpf_sock_ops` |
| 14 | `SK_SKB` | 4.14 | sockmap | `struct __sk_buff` |
| 15 | `CGROUP_DEVICE` | 4.15 | cgroup 设备 | `struct bpf_cgroup_dev_ctx` |
| 16 | `SK_MSG` | 4.17 | sockmap | `struct sk_msg_md` |
| 17 | `RAW_TRACEPOINT` | 4.17 | 追踪 | `struct bpf_raw_tracepoint_args` |
| 18 | `CGROUP_SOCK_ADDR` | 4.18 | cgroup 网络 | `struct bpf_sock_addr` |
| 19 | `LWT_SEG6LOCAL` | 4.18 | SRv6 | `struct __sk_buff` |
| 20 | `LIRC_MODE2` | 4.18 | 红外 | `unsigned int` |
| 21 | `SK_REUSEPORT` | 4.19 | 网络 | `struct sk_reuseport_md` |
| 22 | `FLOW_DISSECTOR` | 4.20 | 网络 | `struct __sk_buff` |
| 23 | `CGROUP_SYSCTL` | 5.2 | cgroup sysctl | `struct bpf_sysctl` |
| 24 | `RAW_TRACEPOINT_WRITABLE` | 5.2 | 追踪 | `struct bpf_raw_tracepoint_args` |
| 25 | `CGROUP_SOCKOPT` | 5.3 | cgroup 网络 | `struct bpf_sockopt` |
| 26 | `TRACING` | 5.5 | 追踪（fentry/fexit） | 依 attach 而定 |
| 27 | `STRUCT_OPS` | 5.6 | 内核子系统（如拥塞控制） | 依具体 ops |
| 28 | `EXT` | 5.6 | 扩展已有程序 | — |
| 29 | `LSM` | 5.7 | 安全 | `struct bpf_ctx` |
| 30 | `SK_LOOKUP` | 5.8 | 网络 | `struct bpf_sk_lookup` |
| 31 | `SYSCALL` | 5.14 | 用户态调用 | `union bpf_attr` 等 |
| 32 | `NETFILTER` | **6.4** | 网络（NF hook） | `struct bpf_nf_ctx` |

> **注意枚举值是稳定的 ABI**：这些数字一旦发布就不能改，所以内核只能「在末尾追加」。这也是为什么列表里出现了 `LIRC_MODE2` 这种和网络毫无关系的成员。

**与网络直接相关的有 17 种**（上表中「领域」列含「网络 / cgroup 网络 / sockmap / 路由隧道 / NF」的）。这也是 [01](01-ebpf-net-bootlin.md) 第 3 节那张横表的来源。

### 1.1 谁有资格加载：`CAP_BPF`

`kernel/bpf/syscall.c:2584-2592` 的加载检查（`BPF_PROG_LOAD`）：

```c
	if (sysctl_unprivileged_bpf_disabled && !bpf_capable())
		return -EPERM;

	if (attr->insn_cnt == 0 ||
	    attr->insn_cnt > (bpf_capable() ? BPF_COMPLEXITY_LIMIT_INSNS : BPF_MAXINSNS))
		return -E2BIG;
	if (type != BPF_PROG_TYPE_SOCKET_FILTER &&
	    type != BPF_PROG_TYPE_CGROUP_SKB &&
	    !bpf_capable())
		return -EPERM;
```

三条规则：

1. **`sysctl_unprivileged_bpf_disabled` = 1 时，非特权一律 `-EPERM`。** 现代发行版（含 Debian/RHEL 默认）基本都是 1。
2. **指令数超限返回 `-E2BIG`，不是 `-EINVAL`**（很多人按 `-EINVAL` 去 grep，永远找不到）。
3. **即使关掉 `unprivileged_bpf_disabled`，无特权进程也只能加载 `SOCKET_FILTER` 和 `CGROUP_SKB` 两种。** 想挂 XDP / tc，必须有 `CAP_BPF` 或 `CAP_SYS_ADMIN`。

`bpf_capable()` 的定义（`include/linux/capability.h:200`）：

```c
static inline bool bpf_capable(void)
{
	return capable(CAP_BPF) || capable(CAP_SYS_ADMIN);
}
```

### 1.2 一个反直觉的细节：特权程序反而「验得更快」

verifier 里 `env->bpf_capable` 会影响精度追踪的开关（`kernel/bpf/verifier.c:2276`）：

```c
	reg->precise = !env->bpf_capable;
```

即：

| 调用者 | `precise` | 含义 |
|--------|-----------|------|
| 有 `CAP_BPF`/`CAP_SYS_ADMIN` | `false` | 默认不做精确标量追踪 → verifier **更宽松、更快** |
| 无特权 | `true` | 强制精确追踪 → verifier **更严格、更慢** |

原因是安全模型：特权代码已经被信任，不需要用 verifier 兜底；非特权代码必须靠 verifier 严格证明。

同理，回边检测处（`kernel/bpf/verifier.c:14761`）：

```c
		if (loop_ok && env->bpf_capable)
			return DONE_EXPLORING;
```

——`bpf_loop()` 这类 helper 构造的循环，**只有特权程序才能用**；无特权程序遇到回边直接 `-EINVAL`。

---

## 2. Map 类型：v6.6 共 33 种

`enum bpf_map_type`（`include/uapi/linux/bpf.h:907-946`），同样 33 种。按用途分四类：

### 2.1 通用键值存储（8 种）

| 类型 | 特点 | 适用 |
|------|------|------|
| `HASH` | 哈希表，动态增删 | 连接跟踪、流表 |
| `ARRAY` | 定长数组，索引即 key | 计数器、配置 |
| `PERCPU_HASH` / `PERCPU_ARRAY` | 每 CPU 独立副本 | **高吞吐计数**（无原子竞争） |
| `LRU_HASH` / `LRU_PERCPU_HASH` | 满了自动淘汰 | 缓存 |
| `LPM_TRIE` | 最长前缀匹配 | **路由表 / IP 前缀过滤** |

### 2.2 特殊语义存储（7 种）

| 类型 | 特点 |
|------|------|
| `PROG_ARRAY` | 尾调用（`bpf_tail_call`）的程序数组 |
| `ARRAY_OF_MAPS` / `HASH_OF_MAPS` | map 中存 map 的 fd，做 map-in-map |
| `QUEUE` / `STACK` | FIFO / LIFO，只能 `push`/`pop`/`peek`，**不能随机访问** |
| `BLOOM_FILTER` | 5.16+，概率型成员查询，省内存 |
| `STRUCT_OPS` | 内核子系统的函数指针表（如自定义 TCP 拥塞控制算法） |

### 2.3 网络专用（7 种）

| 类型 | 用途 | 详细见 |
|------|------|--------|
| `DEVMAP` / `DEVMAP_HASH` | XDP redirect 到网卡 | [chapter-07](../../chapter-07-xdp-redirect-dpdk/notes/01-xdp-redirect.md) |
| `CPUMAP` | XDP redirect 到另一个 CPU | 同上 |
| `XSKMAP` | XDP redirect 到 AF_XDP socket | [chapter-06](../../chapter-06-af-xdp/notes/01-af-xdp.md) |
| `SOCKMAP` / `SOCKHASH` | socket 重定向（sk_msg / sk_skb） | — |
| `REUSEPORT_SOCKARRAY` | `SO_REUSEPORT` 组内的 socket 数组 | — |

### 2.4 存储类（local storage，6 种）+ 事件通道（3 种）

| 类型 | 用途 |
|------|------|
| `SK_STORAGE` | 挂在 `struct sock` 上的 per-socket 私有数据 |
| `INODE_STORAGE` / `TASK_STORAGE` / `CGRP_STORAGE` | 挂在这些内核对象上的私有数据 |
| `CGROUP_STORAGE` / `PERCPU_CGROUP_STORAGE` | cgroup 私有数据（前者已 deprecated，与 `CGROUP_STORAGE_DEPRECATED` 同值） |
| `PERF_EVENT_ARRAY` | perf 事件环形缓冲（per-CPU **且** 每个 buffer 独立，乱序） |
| `RINGBUF` | 5.8+，全局单一环形缓冲，**按提交顺序**，无 per-CPU 拷贝 |
| `USER_RINGBUF` | 6.1+，**用户态生产、内核消费**（方向与 `RINGBUF` 相反） |

> **`RINGBUF` vs `PERF_EVENT_ARRAY` 的选型**：前者保序、内存利用率高、但所有 CPU 竞争同一个 producer 位置；后者 per-CPU 无竞争、但**事件在多个 CPU 间可能乱序**，且每个 CPU 都得预留一份内存。HFT 场景如果要做「按到达顺序重建事件序列」，只能用 `RINGBUF`；如果只做聚合计数，`PERCPU_ARRAY` 最省。

---

## 3. Verifier：三条硬规则 + 真实限流常量

### 3.1 三条硬规则（这个说法是对的）

`kernel/bpf/verifier.c:44-62` 的文档注释：

```c
/* bpf_check() is a static code analyzer that walks eBPF program
 * instruction by instruction and updates register/stack state.
 * All paths of conditional branches are analyzed until 'bpf_exit' insn.
 *
 * The first pass is depth-first-search to check that the program is a DAG.
 * It rejects the following programs:
 * - larger than BPF_MAXINSNS insns
 * - if loop is present (detected via back-edge)
 * - unreachable insns exist (shouldn't be a forest. program = one function)
 * - out of bounds or malformed jumps
 * The second pass is all possible path descent from the 1st insn.
 * Since it's analyzing all paths through the program, the length of the
 * analysis is limited to 64k insn, which may be hit even if total number of
 * insn is less then 4K, but there are too many branches that change stack/regs.
 * Number of 'branches to be analyzed' is limited to 1k
 */
```

三条硬规则：**程序必须是 DAG（不能有回边）**、**不能有不可达指令**、**跳转不能越界或畸形**。

配套的具体约束（都是 XDP/tc 程序天天会撞到的）：

- **包边界检查**：`data + offset + size > data_end` 必须在使用前显式判掉，否则 `R? offset is outside of the packet`。
- **栈大小**：`MAX_BPF_STACK = 512` 字节（`include/linux/filter.h:90`）。
- **调用深度**：`MAX_CALL_FRAMES 8`（`include/linux/bpf_verifier.h:316`）。
- **可变偏移范围**：`BPF_MAX_VAR_OFF = BPF_MAX_VAR_SIZ = 1 << 29`（`include/linux/bpf_verifier.h:16,20`），即 512 MB。
- **寄存器**：R0 返回值，R1–R5 传参，R6–R9 callee-saved，R10 只读帧指针（见 verifier.c:66-73 注释）。**R1 进入时是 `PTR_TO_CTX`。**

### 3.2 ⚠️ 三处「文档与代码不一致」，必须纠正

#### 纠正 1：第二遍的限流是 **100 万**，不是 64k

文档注释说「the analysis is limited to 64k insn」。**v6.6 代码里不是这样。** 实际执行点在 `kernel/bpf/verifier.c:16455`：

```c
		if (++env->insn_processed > BPF_COMPLEXITY_LIMIT_INSNS) {
			verbose(env,
				"BPF program is too large. Processed %d insn\n",
				env->insn_processed);
			return -E2BIG;
		}
```

而 `BPF_COMPLEXITY_LIMIT_INSNS` 是（`include/linux/bpf.h:1723`）：

```c
#define BPF_COMPLEXITY_LIMIT_INSNS      1000000 /* yes. 1M insns */
```

**所以：第二遍路径分析的预算是 100 万条「被处理的指令」，不是 64k。** 这个预算是**累计处理量**，不是程序长度——一个 500 条指令但有大量分支的程序，可以轻松把 `insn_processed` 推到 100 万。

报错信息是 `BPF program is too large. Processed %d insn`，返回 `-E2BIG`。

#### 纠正 2：「branches 限制 1k」已经不存在

注释里的 `Number of 'branches to be analyzed' is limited to 1k` 在 v6.6 代码里**找不到对应的限流**。`struct bpf_verifier_state` 里的 `u32 branches` 字段（`include/linux/bpf_verifier.h:368`）另有用途——它是**「还剩多少条路径没探索」的计数**，用于检测无限循环：

```c
	/*
	 * 'branches' field is the number of branches left to explore:
	 * 0 - all possible paths from this state reached bpf_exit or
	 *     were safely pruned
	 * 1 - at least one path is being explored.
	 * ...
	 * If is_state_visited() sees a state with branches > 0 it means
	 * there is a loop. ...
	 * This algorithm may not find all possible infinite loops or
	 * loop iteration count may be too high.
	 * In such cases BPF_COMPLEXITY_LIMIT_INSNS limit kicks in.
	 */
	u32 branches;
```

注意最后一句：**「漏检的循环最终靠 `BPF_COMPLEXITY_LIMIT_INSNS` 兜底」**——也就是说这个 100 万指令预算**同时也是循环检测的兜底机制**。写 verifier 日志里出现 `BPF program is too large` 时，原因可能是程序真的太大，也可能是**写了一个 verifier 没识别出来的循环**。

#### 纠正 3：`BPF_MAXINSNS` 确实存在，且是 4096

上一版笔记和很多资料说「4096」，但没给出处。`BPF_MAXINSNS` 定义在 **`include/uapi/linux/bpf_common.h:53-54`**（不在 `bpf.h`，也不在 `filter.h`，这也是为什么大家 grep 不到）：

```c
#ifndef BPF_MAXINSNS
#define BPF_MAXINSNS 4096
#endif
```

完整的两级指令数上限：

| 调用者 | 单程序最大指令数 | 位置 |
|--------|----------------|------|
| 有 `CAP_BPF`/`CAP_SYS_ADMIN` | **1,000,000**（`BPF_COMPLEXITY_LIMIT_INSNS`） | `include/linux/bpf.h:1723` |
| 无特权 | **4,096**（`BPF_MAXINSNS`） | `include/uapi/linux/bpf_common.h:54` |

超限在 `BPF_PROG_LOAD` 阶段返回 `-E2BIG`（`kernel/bpf/syscall.c:2587`）。

### 3.3 v6.6 里真实存在的其他限流常量

`kernel/bpf/verifier.c:183-184`：

```c
#define BPF_COMPLEXITY_LIMIT_JMP_SEQ	8192
#define BPF_COMPLEXITY_LIMIT_STATES	64
```

| 常量 | 值 | 作用 |
|------|-----|------|
| `BPF_COMPLEXITY_LIMIT_JMP_SEQ` | 8192 | 单条「跳转序列」的复杂度上限 |
| `BPF_COMPLEXITY_LIMIT_STATES` | 64 | 剪枝点的候选状态数上限（超出就不再尝试 `states_equal()` 剪枝） |
| `BPF_COMPLEXITY_LIMIT_INSNS` | 1,000,000 | 加载时上限 + 第二遍路径分析预算（**双重用途**） |
| `MAX_CALL_FRAMES` | 8 | 子程序调用深度 |
| `MAX_BPF_STACK` | 512 字节 | BPF 栈 |

### 3.4 完整报错速查

| verifier 报错 | 真实原因 | 对策 |
|--------------|---------|------|
| `R? offset is outside of the packet` | 包边界检查缺失或写法让 verifier 无法推断 | 在解引用前 `if ((void *)(hdr + 1) > data_end) return XDP_PASS;` |
| `BPF program is too large. Processed %d insn` | `insn_processed` 超 100 万；**也可能是未识别的循环** | 拆分逻辑、减少分支、用尾调用；或检查是否写出隐式循环 |
| `back-edge from insn %d to %d` | 检测到回边（循环） | 展开循环为固定次数；或用 `bpf_loop()`（需特权） |
| `invalid bpf_context access` | 访问了上下文结构体中不存在的字段（常见于 CO-RE 或类型不匹配） | 确认程序类型与上下文类型匹配；用 `bpftool btf dump` 核对字段 |
| `jump out of range` | 跳转目标越界 | 编译器问题，检查是否手写了 `.S` |
| `unreachable insn %d` | 存在不可达指令 | 通常是编译器产生的死代码，升级 clang |
| `call to invalid bpf_func_id` / `unknown func` | 该程序类型不支持这个 helper | 查 [01](01-ebpf-net-bootlin.md) 第 3.1 节的 helper 能力集表 |
| `R1 type=scalar expected=ctx` | 把 R1 当普通寄存器用了 | R1 进入时是 `PTR_TO_CTX`，别覆盖它 |

---

## 4. helper 能力集：程序类型决定你能调什么

`net/core/filter.c` 里各 `get_func_proto` 的 `case BPF_FUNC_*` 分支数（v6.6 实测）：

| 分发函数 | 源码位置 | `case` 数 | 兜底 |
|---------|---------|----------|------|
| `tc_cls_act_func_proto()` | :7968 | **81** | `bpf_sk_base_func_proto()` |
| `xdp_func_proto()` | :8084 | **23** | `bpf_sk_base_func_proto()` |
| `cg_skb_func_proto()` | :7919 | 15 + `cgroup_common_func_proto()` | `sk_filter_func_proto()` |
| `sk_msg_func_proto()` | :8226 | 14 | `bpf_sk_base_func_proto()` |
| `sock_ops_func_proto()` | :8178 | 14 | `bpf_sk_base_func_proto()` |
| `sk_filter_func_proto()` | :7897 | **5** | `bpf_sk_base_func_proto()` |

### 4.1 XDP 的 23 个 helper（`xdp_func_proto()`，net/core/filter.c:8084）

| 类别 | helper |
|------|--------|
| 包改写 | `xdp_adjust_head`、`xdp_adjust_tail`、`xdp_adjust_meta` |
| 包访问 | `xdp_load_bytes`、`xdp_store_bytes`、`xdp_get_buff_len` |
| 转发 | `redirect`、`redirect_map` |
| 校验和 | `csum_diff` |
| 查表 | `fib_lookup`、`check_mtu` |
| socket 关联 | `sk_lookup_tcp`、`sk_lookup_udp`、`skc_lookup_tcp`、`sk_release` |
| syncookie | `tcp_check_syncookie`、`tcp_gen_syncookie`、`tcp_raw_{gen,check}_syncookie_ipv4`、`tcp_raw_{gen,check}_syncookie_ipv6` |
| 输出/杂项 | `perf_event_output`、`get_smp_processor_id` |

**XDP 拿不到的**（这是选型时的硬约束）：所有 `skb_*` 系列（没有 skb）、`bpf_redirect_neigh`（依赖邻居子系统）、`bpf_clone_redirect`、`bpf_sk_assign`、`bpf_skb_change_tail`、`bpf_get_socket_cookie`（无 socket）等等。

### 4.2 tc 的 81 个 helper（`tc_cls_act_func_proto()`，net/core/filter.c:7968）

比 XDP 多出的关键能力：

| 能力 | helper | 为什么 XDP 不能有 |
|------|--------|-----------------|
| 邻居转发 | `redirect_neigh`、`redirect_peer` | 需要 `struct neighbour`，属于 skb/路由层 |
| 克隆转发 | `clone_redirect` | 需要 skb 的引用计数 |
| 包长度调整 | `skb_change_tail`、`skb_adjust_room`、`skb_change_head` | 需要 skb 的线性/非线性区管理 |
| 校验和增量更新 | `l3_csum_replace`、`l4_csum_replace`、`csum_update`、`csum_level` | XDP 只有 `csum_diff`（算差值，不写回） |
| VLAN | `skb_vlan_push`、`skb_vlan_pop` | skb 元数据 |
| socket 绑定 | `sk_assign` | 需要 socket 查找后的目标 |
| 隧道 | `skb_get/set_tunnel_key`、`skb_get/set_tunnel_opt` | skb metadata |
| hash | `set_hash`、`set_hash_invalid`、`get_hash_recalc` | skb 元数据 |

> **选型结论**：要「改完包再转发出去」→ tc；要「尽早丢或尽早转」→ XDP。二者的 helper 集合差异不是偶然，是**有没有 skb** 的直接后果。

---

## 5. HFT 要点

1. **指令数上限对 HFT 程序基本不是约束**（特权下 100 万条），真正的约束是 **`env->insn_processed` 的路径分析预算** 和 **verifier 的加载耗时**。一个分支很多的行情解析程序，即使只有 800 条指令，也可能把 `insn_processed` 推爆。判据是报错信息 `Processed %d insn` 里的数字。
2. **`BPF program is too large` 不一定是程序大，可能是循环。** 这是最浪费时间的报错之一：verifier 的循环检测（`branches > 0` + `states_maybe_looping()`）是启发式的，漏检时靠指令预算兜底。遇到这个报错先查循环，再查大小。
3. **map 选型直接决定竞争开销**：高频计数一律用 `PERCPU_ARRAY` / `PERCPU_HASH`（无原子竞争）；需要保序的事件流用 `RINGBUF`；`PERF_EVENT_ARRAY` 会乱序。
4. **别指望 unprivileged BPF**：现代发行版默认 `sysctl_unprivileged_bpf_disabled=1`，且就算关掉也只允许 `SOCKET_FILTER` 和 `CGROUP_SKB` 两种类型。生产部署一律 `CAP_BPF`，不要给 `CAP_SYS_ADMIN`（后者权限大得多）。
5. **栈只有 512 字节**。想在 BPF 里暂存一个包头做解析？放栈上没问题；想暂存一张表？必须放 map。超过 512 字节会直接 `invalid stack` 类的报错。
6. **子程序调用最多 8 层**（`MAX_CALL_FRAMES`）。写深度嵌套的协议解析时要注意，用尾调用（`PROG_ARRAY`）可以突破这个限制但会丢掉寄存器状态。

---

## 6. 与 Rosen 3.x 的差异

| 维度 | Rosen 3.x（classic BPF） | v6.6（eBPF） |
|------|-------------------------|-------------|
| 类型数 | 1（socket filter） | 33 种程序类型 + 33 种 map 类型 |
| 指令集 | 32 位，2 个寄存器（A/X），scratch memory 16 字 | 64 位，11 个寄存器（R0–R10）+ 512 字节栈 |
| JIT | 有（x86 从 3.0 起） | 有，且每个 arch 都有 |
| 校验 | 只检查 jump 是否越界 | verifier 全路径静态分析（DAG / 类型追踪 / 边界 / 全路径） |
| 写能力 | 只读 | 可改包头、可写 map、可重定向 |
| 子程序 | 无 | 有（`BPF_PSEUDO_CALL`，最多 8 层）+ 尾调用 |
| 权限 | `CAP_NET_ADMIN`（`SO_ATTACH_BPF`） / `CAP_NET_RAW`（抓包） | `CAP_BPF`（专用，权限面小得多）+ 类型白名单 |

Rosen 时代的 BPF 是「tcpdump 过滤器语法的后端」；eBPF 是一台独立的、带内存安全证明的虚拟机。名字一样，东西完全不同。

---

## 7. 代码自测

<details>
<summary>Q1：你把 XDP 程序写得很大（很多层的协议解析 + 大量分支），<code>BPF_PROG_LOAD</code> 返回 <code>-E2BIG</code>，日志说 <code>BPF program is too large. Processed 1000001 insn</code>。程序实际只有 900 条指令。为什么？</summary>

**因为 100 万是「被处理的指令数」，不是程序长度。**

看 `kernel/bpf/verifier.c:16455`：

```c
		if (++env->insn_processed > BPF_COMPLEXITY_LIMIT_INSNS) {
			verbose(env,
				"BPF program is too large. Processed %d insn\n",
				env->insn_processed);
			return -E2BIG;
		}
```

`env->insn_processed` 是**第二遍（全路径下降）中累计走过的指令条数**。verifier 要遍历**所有可能的分支组合**，每走一条指令就 +1。

一个 900 条指令、有 N 个二分支的程序，最坏路径数是 2^N。即使有状态剪枝（`is_state_visited()` + `states_equal()`），只要每个分支都改变了栈或寄存器状态，剪枝就会失效，路径数呈指数增长。

**这条限制的另一个身份是循环检测的兜底。** `include/linux/bpf_verifier.h:346-367` 的注释说得明白：

```
 * This algorithm may not find all possible infinite loops or
 * loop iteration count may be too high.
 * In such cases BPF_COMPLEXITY_LIMIT_INSNS limit kicks in.
```

所以排查顺序应该是：

1. **先查循环。** 有没有用 `goto` 往回跳？有没有数组索引驱动的间接跳转？有没有 `bpf_loop()` 之外构造的循环？日志里如果有 `back-edge from insn %d to %d` 就是明确的信号（`verifier.c:14764`）。
2. **再查分支爆炸。** 把大的 `if/else if` 链改成 map 查表（比如用 `BPF_MAP_TYPE_HASH` 做 ethertype → handler 的分派），或拆成多个 tail call。
3. **第三查剪枝失效。** 如果每个分支都在往栈上写不同的东西，verifier 就无法判定状态等价。尽量让分支只改寄存器、不改栈。

**顺便纠正一个流传很广的错误说法**：很多资料（包括内核自己的 `verifier.c:50` 注释）说第二遍「limited to 64k insn」和「branches limited to 1k」。**v6.6 代码里这两个数字都不存在**：实际是 `BPF_COMPLEXITY_LIMIT_INSNS = 1000000`（`include/linux/bpf.h:1723`），而 `branches` 字段（`include/linux/bpf_verifier.h:368`）是「剩余待探索路径数」的计数，用于判环，不是限流阈值。

</details>

<details>
<summary>Q2：你的同事说他写的 BPF 程序「只有 4000 条指令」所以不受限制，因此拒绝了你的拆分建议。他说得对吗？</summary>

**不对，而且他混淆了两个完全不同的限制。**

v6.6 有两级指令数上限，都由 `kernel/bpf/syscall.c:2586-2587` 检查：

```c
	if (attr->insn_cnt == 0 ||
	    attr->insn_cnt > (bpf_capable() ? BPF_COMPLEXITY_LIMIT_INSNS : BPF_MAXINSNS))
		return -E2BIG;
```

| 限制 | 值 | 卡的是 | 谁会撞到 |
|------|-----|-------|---------|
| `BPF_MAXINSNS` | 4,096 | **程序本身的指令条数**（`attr->insn_cnt`） | **只有无特权调用者** |
| `BPF_COMPLEXITY_LIMIT_INSNS` | 1,000,000 | ① 特权调用者的程序长度上限<br>② verifier 第二遍的**累计处理指令数** | **所有人都可能撞到 ②** |

`BPF_MAXINSNS` 定义在 **`include/uapi/linux/bpf_common.h:53-54`**（注意：不在 `bpf.h`，这是很多人 grep 不到的原因）：

```c
#ifndef BPF_MAXINSNS
#define BPF_MAXINSNS 4096
#endif
```

**关键区分：**

- 如果你们用 `CAP_BPF` 加载（生产环境必然如此），那 4096 **根本不适用**，加载时上限是 100 万。他的「只有 4000 条」在加载阶段毫无意义。
- 真正会卡住他的是 `env->insn_processed` 的 100 万预算，而这个和程序的**静态长度**没有直接关系，只和**分支路径数**有关。

**所以正确的表达是**：「加载阶段的 4096 限制跟我们无关，但路径分析的 100 万预算跟分支数有关，我们的程序分支很多，得拆。」

另外提醒两个细节：

- 这两个限制超限都返回 **`-E2BIG`**，不是 `-EINVAL`。
- 即使把 `kernel.unprivileged_bpf_disabled` 设成 0，**无特权进程也只允许加载 `SOCKET_FILTER` 和 `CGROUP_SKB`**（`kernel/bpf/syscall.c:2588-2591`）。想挂 XDP / tc 必须有 `CAP_BPF`（`include/linux/capability.h:200`：`capable(CAP_BPF) || capable(CAP_SYS_ADMIN)`）。

</details>

<details>
<summary>Q3：你在 tc-BPF 程序里调用了 <code>bpf_redirect_neigh()</code> 做转发，verifier 通过了。后来想把这段逻辑下沉到 XDP 里省掉 skb，结果 <code>bpf_redirect_neigh()</code> 报 <code>unknown func</code>。为什么 XDP 用不了它？</summary>

**因为能力集是按程序类型静态分发的，而 XDP 的 helper 表里根本没有这个 helper。**

看 `net/core/filter.c` 的两张表：

```c
/* tc：net/core/filter.c:7968 */
	case BPF_FUNC_redirect_neigh:
		return &bpf_redirect_neigh_proto;
	case BPF_FUNC_redirect_peer:
		return &bpf_redirect_peer_proto;
	...
```

而 XDP 的 `xdp_func_proto()`（`net/core/filter.c:8084`）里，转发相关的**只有**：

```c
	case BPF_FUNC_redirect:
		return &bpf_xdp_redirect_proto;
	case BPF_FUNC_redirect_map:
		return &bpf_xdp_redirect_map_proto;
```

**根本原因不是「还没实现」，而是语义上做不到。** `bpf_redirect_neigh()` 的作用是「查邻居表拿到 L2 地址，然后转发出去」，它需要：

1. `struct neighbour` / `struct dst_entry` —— 属于路由子系统，XDP 层还没有做路由查找；
2. 一个 `sk_buff` 来承载 L2 头改写和后续 qdisc 排队。

而 XDP 执行时**既没有 skb 也没有路由结果**（`bpf_prog_run_xdp()` 在 `include/net/xdp.h:482`，此时包还只是 DMA 区域里的一段内存）。

**这是 XDP 与 tc 能力边界的本质差异：**

| | XDP（23 helper） | tc（81 helper） |
|---|---|---|
| 有没有 skb | ❌ 只有 `xdp_buff` | ✅ 完整 `sk_buff` |
| 路由/邻居 | ❌ | ✅ `fib_lookup`、`redirect_neigh`、`redirect_peer` |
| 改包长度 | `xdp_adjust_head/tail`（只能缩/扩头部边界） | `skb_change_tail`、`skb_adjust_room`、`skb_change_head`（可任意增删） |
| 校验和 | `csum_diff`（只算差值，不写回） | `l3/l4_csum_replace`、`csum_update`（增量写回） |
| 克隆 | ❌ | `clone_redirect` |
| VLAN | ❌ | `skb_vlan_push/pop` |

**给 HFT 的选型结论：**

- 想「尽早丢包 / 尽早分流到 AF_XDP / 尽早做 XDP_TX 回弹」→ **XDP**，接受 helper 少的约束。
- 想「改包内容 + 查邻居 + 转发 / 做隧道 / 改 skb 元数据」→ **tc**，接受已经付了 skb 分配的代价。
- 想两者兼得 → 混合架构：XDP 做快路径（丢包 + AF_XDP 分流），`XDP_PASS` 上去的慢路径交给 tc 处理。这是 Cilium 等项目的标准做法，详见 [chapter-09-tc-bpf](../../chapter-09-tc-bpf/)。

</details>

---

## 导航

- **本篇：** [01-ebpf-net-bootlin.md](01-ebpf-net-bootlin.md) hook 全景与工具链 · [03-xdp-bpf.md](03-xdp-bpf.md) · [04-cgroup-bpf.md](04-cgroup-bpf.md)
- **相关：** [chapter-05-xdp-architecture](../../chapter-05-xdp-architecture/) · [chapter-07-xdp-redirect-dpdk](../../chapter-07-xdp-redirect-dpdk/) · `06.7-bpf-observability/`
- **章节主页：** [README](../README.md)
