# 01 — 包过滤器：从 cBPF 到 eBPF，与 socket filter 的真实位置

> **对应 Rosen:** Ch9（Netfilter）/ Ch1（socket filter）
> **内核源码路径：** `Documentation/networking/filter.rst`
> **核对源码：** v6.6 `net/core/filter.c`（转换与挂载链路）、`include/uapi/linux/filter.h`、`include/uapi/linux/bpf_common.h`

## 文档概述

`filter.rst` 是内核对「包过滤器」的官方总账：cBPF（classic BPF）怎么来的、eBPF 怎么扩展的、两者怎么共存。本篇在文档基础上补齐**源码级链路**：cBPF 程序塞进内核后到底发生了什么、socket filter 挂在哪、为什么它是「最晚的过滤点」。

姊妹篇分工：

| 文件 | 主题 | 与本篇的关系 |
|------|------|-------------|
| [02-nf-flowtable.md](02-nf-flowtable.md) | Netfilter flowtable：连接级快路径 | 本篇讲「包级过滤器的演进」，02 讲「流级缓存的思路」——同一动机（慢路径太贵）的两种解法 |

---

## 1. cBPF：1992 年的设计，至今还在用

cBPF（classic BPF，来自 BSD Packet Filter，McCanne & Jacobson 1992）是**为 tcpdump 而生**的：

- **两个 32 位寄存器**：A（累加器）、X（变址）
- **一个 16 位字长的栈**（`BPF_MEMWORDS = 16`）
- 指令集：`load`（从包的固定偏移取数）→ `jump`（条件/无条件）→ `ret`（给裁决）

```c
/* include/uapi/linux/bpf_common.h —— 指令编码 */
struct sock_filter {	/* Filter block */
	__u16	code;	/* Actual filter code */
	__u8	jt;	/* Jump true */
	__u8	jf;	/* Jump false */
	__u32	k;	/* Generic multiuse field */
};

struct sock_fprog {	/* Required for SO_ATTACH_FILTER. */
	unsigned short		len;
	struct sock_filter __user *filter;
};
```

**今天还在用它的地方**：tcpdump/libpcap 的过滤器表达式（`tcp port 9090`）编译成的就是 cBPF；`SO_ATTACH_FILTER` 接收的也是 cBPF。**不是因为好，而是因为兼容**——libpcap 的 ABI 覆盖了所有 Unix。

### 1.1 cBPF 的返回值语义（容易搞错）

```c
BPF_STMT(BPF_RET | BPF_K, 65535);   /* 返回 65535 = 「接受整个包」 */
BPF_STMT(BPF_RET | BPF_K, 0);       /* 返回 0 = 「丢弃」 */
BPF_STMT(BPF_RET | BPF_K, 64);      /* 返回 64 = 「接受前 64 字节」 */
```

**返回值是「保留的字节数」，不是布尔裁决**。返回 64 意味着内核只把前 64 字节放进 socket 队列（snapshot length 语义，tcpdump 的 `-s` 就是这么实现的）。

---

## 2. ⭐ v6.6 的真相：cBPF 在内核里已被「翻译执行」

**内核里没有 cBPF 解释器了**（对有 JIT 的平台）。`SO_ATTACH_FILTER` 塞进来的 cBPF 会被翻译成 eBPF 再执行：

```c
/* net/core/filter.c:1242 —— 挂载路径上的一环 */
static struct bpf_prog *bpf_migrate_filter(struct bpf_prog *fp)
{
	...
	/* 1st pass: 计算翻译后的指令数 */
	err = bpf_convert_filter(old_prog, old_len, NULL, &new_len, ...);
	/* 2nd pass: 真正翻译 */
	err = bpf_convert_filter(old_prog, old_len, fp, &new_len, ...);
```

翻译器 `bpf_convert_filter()`（`net/core/filter.c:559`）做的事：

| cBPF 概念 | eBPF 对应 |
|---|---|
| A/X 寄存器 | 函数序言里把 `BPF_REG_0`/`BPF_REG_1` 初始化为 0（「Classic BPF expects A and X to be reset first」，源码注释） |
| `BPF_LD \| BPF_ABS`（从包固定偏移取数） | `BPF_LD \| BPF_W \| BPF_ABS`（eBPF 的 64 位变体，语义继承） |
| 16 个 scratch 内存字 | eBPF 栈（`MAX_BPF_STACK = 512` 字节里划区） |
| 条件跳转 jt/jf（两个目标） | eBPF 单目标条件跳转 + 反转（每条 cBPF 条件跳转可能膨胀成两条 eBPF 指令） |
| `ret k` | eBPF `mov r0, k; exit` |

翻译后走和原生 eBPF 完全一样的路：**verifier 审查 → JIT 编译成原生机器码**。

**实操含义**：

1. tcpdump 的过滤器在 v6.6 上跑的是 **JIT 过的 eBPF**——「cBPF 慢」的旧知识已过时
2. `BPF_MAXINSNS = 4096`（`bpf_common.h:53-54`）限制的是**翻译前**的 cBPF 指令数（`len > BPF_MAXINSNS` → `-EINVAL`，`bpf_convert_filter` 开头检查）
3. cBPF 的跳转是「包内数据驱动的稀疏匹配」模型，eBPF 是「通用计算」模型——前者表达不了循环和状态，这是当年的安全设计，也是它被取代的原因

---

## 3. socket filter 的完整挂载链路

```c
/* 两种挂载方式 */
setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &sock_fprog, ...);  /* cBPF：结构体 */
setsockopt(fd, SOL_SOCKET, SO_ATTACH_BPF, &bpf_fd, ...);          /* eBPF：文件描述符 */
```

| | `SO_ATTACH_FILTER` | `SO_ATTACH_BPF` |
|---|---|---|
| 输入 | `sock_fprog`（cBPF 指令数组） | eBPF 程序的 fd（先 `bpf()` 加载） |
| 内核处理 | `bpf_migrate_filter()` 翻译 → verifier → JIT | 直接 verifier → JIT |
| 能力 | 只能读包头固定偏移 | 完整 eBPF：map、helper |
| 进程关系 | 程序嵌入 socket | fd 引用，可多 socket 共享 |

### 3.1 挂载过程（`net/core/filter.c`）

```
sk_attach_filter()
  ├─ bpf_prog_get_type_dev()          /* 取 eBPF 程序 */
  ├─ __sk_filter_charge(sk, fp)       /* ⭐ 内存配额检查 */
  └─ rcu_assign_pointer(sk->sk_filter, fp)   /* RCU 替换，旧程序 grace period 后释放 */
```

`__sk_filter_charge()`（filter.c:1216）限制单个 socket 上的 filter 程序总内存（`sk_filter` 配额，防 fork 炸弹式资源耗尽——每个子进程继承 socket 时都要 charge）。

### 3.2 执行位置：socket 入队口的 `sk_filter_trim_cap()`

这个位置在 [chapter-08](../../chapter-08-ebpf-cgroup-bpf/) 已核对过（`net/core/filter.c:124/138`），这里给全链：

```
收包：... → IP 栈 → UDP/TCP → socket 查找 → sock_queue_rcv_skb()
                                                  │
                                                  ▼
                                       sk_filter_trim_cap()   ← socket filter 在这跑
                                                  │
                                          ├─ 返回 0 → 入队（可截断到返回值长度）
                                          └─ 返回 非0 → 丢弃（EPERM）
                                                  ▼
                                       接收队列 → 唤醒应用 → recvmsg()
```

**注意已经付出了什么**：完整协议栈解析、conntrack（若启用）、路由、socket 查找全做完了。socket filter 省的只是「协议栈处理后的排队和唤醒」。

### 3.3 socket filter 的能力残缺（v6.6 实测）

eBPF socket filter（`BPF_PROG_TYPE_SOCKET_FILTER`）虽然是 eBPF，但 helper 只有 **5 个**（对照 tc 的 81 个），且：

- ❌ **禁访 `data`/`data_end`**——读不到包内容（只能看 `len`/`protocol` 等元数据）
- ❌ 不能写任何 skb 字段（除 `cb[]`）
- ✅ 能查 map（LPM trie 做白名单是唯一有点意思的用法）

这是位置决定的：它运行在 `sk_filter_trim_cap()`，这个点的语义就是「这个包要不要交给这个 socket」的二元裁决，内核没给它更多权限。（详细对比表见 [chapter-09 的 02 篇 §4.2](../../chapter-09-tc-bpf/notes/02-tc-bpf.md)。）

---

## 4. 位置对比：五个「过滤点」的完整账单

| 过滤点 | 位置 | 已付出的成本（被丢的包白白消耗） | 拿到什么 | 程序类型 |
|---|---|---|---|---|
| XDP | 驱动层，skb 分配前 | 仅 DMA + 驱动中断/NAPI | `xdp_buff`（线性区） | `BPF_PROG_TYPE_XDP` |
| tc ingress | `__netif_receive_skb_core` :5412 | + skb 分配、ptype_all | `__sk_buff`（可改） | `SCHED_CLS` |
| nft INPUT | IP 层 LOCAL_IN | + VLAN 剥层、IP 校验、路由、conntrack | 完整 skb 上下文 | nft 表达式 |
| socket filter | `sock_queue_rcv_skb` | + 完整 UDP/TCP 栈、socket 查找 | socket 局部视图 | `SOCKET_FILTER` |
| recvmsg 后过滤 | 用户态 | + 排队、唤醒、上下文切换、系统调用 | 任意（用户代码） | 应用代码 |

**HFT 的结论不变**：过滤每往后挪一层，被丢流量浪费的 CPU 就多一截。socket filter 在这个序列里**只比「不过滤」强一点**——它的正当用途是「同一台机上多进程共享 socket，某进程只想要子集」这种粗筛，而不是行情过滤的主力。

---

## 5. tcpdump 的一个实用细节

tcpdump 自己就是 socket filter 的最大用户（`AF_PACKET` + `SO_ATTACH_FILTER`）：

- **过滤器在 tap 之前生效的部分**：libpcap 编译的 cBPF 在内核里跑（`PACKET_FILTER`），不匹配的包**根本不会拷贝到用户态**——这就是「tcpdump 带过滤比不带过滤对系统影响小」的原因
- **位置在 ptype_all**（dev.c:5394）：所有 Netfilter/tc/XDP 之外的处理之前。被 tc/nft/cgroup 丢的包 tcpdump 都看得到；被 XDP 丢的看不到
- 抓包本身有成本：每个匹配包都要 `skb_clone` + 用户态拷贝。**生产环境长开 tcpdump 过滤器要收窄**，别 `tcpdump -i eth0` 全抓

---

## 6. HFT 要点

1. **cBPF 已是「历史接口」**：v6.6 里它被翻译成 eBPF 执行，性能与手写 eBPF 同级（JIT 后），但能力是子集——新代码没有理由再写 cBPF
2. **socket filter 是最晚的内核过滤点**，省的只是排队+唤醒；行情早过滤必须在 XDP
3. **`SO_ATTACH_BPF` + map** 是 socket filter 唯一的现代玩法：用户态服务实时更新白名单 map，filter 只查 map 裁决
4. **tcpdump 的内核态预过滤**（libpcap → cBPF → 翻译 → JIT）是低成本观测方案，比全抓+用户态过滤便宜一个数量级
5. **返回值语义**：cBPF 返回的是截断长度不是布尔值——`return 0` 才是丢，`return -1`（或大值）是收

---

## 7. 与 Rosen 的差异

| 维度 | Rosen 3.x | v6.6 |
|---|---|---|
| cBPF 执行 | 独立解释器（`sk_run_filter`） | **翻译成 eBPF** → verifier → JIT |
| eBPF | 无（书里不会有） | 33 种程序类型的体系（见 [chapter-08](../../chapter-08-ebpf-cgroup-bpf/)） |
| socket filter 能力 | 读包头 | 元数据 only（连 data 都禁） |
| tcpdump 过滤 | 解释执行 | JIT 执行 |

---

## 8. 代码自测

<details>
<summary>Q1：`BPF_STMT(BPF_RET | BPF_K, 128)` 在 socket filter 里是什么意思？</summary>

「接受这个包，但只保留前 128 字节进接收队列」。cBPF 的返回值是 **snapshot length**：

- `0` → 丢包
- `k > 0` → 保留 min(k, len) 字节（tcpdump `-s 128` 就是生成这个）

不是布尔语义。eBPF socket filter（`SO_ATTACH_BPF`）继承了同样语义：返回值 = 保留字节数。
</details>

<details>
<summary>Q2：tcpdump 的过滤器在 v6.6 内核里以什么形式执行？</summary>

**JIT 编译的 eBPF 机器码**。链路：libpcap 把 `tcp port 9090` 编译成 cBPF → `SO_ATTACH_FILTER`（`AF_PACKET` socket 上是 `PACKET_FILTER`）→ `bpf_migrate_filter()`（filter.c:1242）→ `bpf_convert_filter()` 两遍翻译成 eBPF → verifier → JIT。

所以「cBPF 是解释执行所以慢」的说法在 v6.6 上不成立；过滤开销主要在**匹配包的 clone + 用户态拷贝**，不在过滤器执行本身。
</details>

<details>
<summary>Q3：为什么 eBPF socket filter 连包内容都读不到？</summary>

程序类型的能力由**执行位置的语义**决定。socket filter 跑在 `sk_filter_trim_cap()`（`net/core/filter.c:124`），是 socket 接收队列入口的「该不该入队」裁决器。内核为它定义的访问规则（`bpf_base_func_proto` + `sk_filter_func_proto`）里没有授权 `data`/`data_end` 直接访存——`bpf_skb_is_valid_access()` 对 SOCKET_FILTER 类型拒绝这两个字段。

设计意图：入队裁决不需要看内容（看 len/protocol 就够）；要看内容的过滤应该在 tc/XDP 做，那里有完整的包访问授权。
</details>

<details>
<summary>Q4：多进程共享一个 UDP socket，进程 A 只要端口 5000 的包。用 socket filter 还是 XDP？</summary>

**socket filter 就够了**（前提：过滤条件是元数据可见的）。但注意端口对 socket filter 是「本 socket 的端口」，多进程共享 socket 意味着看到的都是同端口——「只要端口 5000」这个条件在 socket 层已经没有区分度了，实际得按**包内容/源地址**分。

如果按源 IP 分：`SO_ATTACH_BPF` + LPM trie map（用户态可实时更新）是正解——这是 socket filter 少数有意义的现代用法。XDP 在这里杀鸡用牛刀（XDP 丢弃是全 socket 的，你只想对 A 的队列过滤，XDP 做不到「按进程」）。

**陷阱**：SO_ATTACH_BPF 是 socket 级别的，同一 socket 上 A/B 两进程都会受影响。多进程各开 socket + SO_REUSEPORT 才能各自挂各自的 filter。
</details>

---

## 导航

- **下一篇：** [02-nf-flowtable.md](02-nf-flowtable.md) — Netfilter flowtable：把「首包慢路径」和「后续包快路径」分开的连接级缓存
- **相关：** [chapter-08-ebpf-cgroup-bpf/](../../chapter-08-ebpf-cgroup-bpf/) eBPF 类型系统与 socket filter 能力残缺的对比 · [chapter-10-nftables/](../../chapter-10-nftables/) nftables hook 体系 · [chapter-05-xdp-architecture/](../../chapter-05-xdp-architecture/) XDP（真正的早过滤）
- **章节主页：** [README](../README.md)
