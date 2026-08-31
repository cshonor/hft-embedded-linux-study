# 02 — nftables 内部机制：求值引擎、规则 blob 与 set 后端

> **来源：** LWN nftables 系列 + **v6.6 源码逐行核对**
> （`net/netfilter/nf_tables_core.c`、`net/netfilter/nf_tables_api.c`、
> `net/netfilter/nft_set_{hash,rbtree,pipapo}.c`、`include/net/netfilter/nf_tables.h`、
> `include/uapi/linux/netfilter/nf_tables.h`）
> **对应 Rosen:** Ch9（无此内容——3.x 时代 iptables 是 match/target 回调链，没有 VM）
> **内核版本：** nftables 3.13+；本篇机制以 **v6.6** 为准

## 文档概述

本篇拆开 nftables 的发动机舱：**一条规则在内核里怎么跑**（求值引擎）、**规则集怎么原子替换**（双代际 blob）、**集合为什么快**（三后端 + 选择算法）。

姊妹篇分工：

| 文件 | 主题 | 与本篇的关系 |
|------|------|-------------|
| [01-nftables-bootlin.md](01-nftables-bootlin.md) | hook 体系、优先级、verdict | 01 讲「挂哪」，本篇讲「跑起来什么样」 |
| [03-nftables-vs-bpf.md](03-nftables-vs-bpf.md) | 与 XDP/tc-BPF 的选型对比 | 本篇的引擎成本数据是 03 对比的依据 |

---

## 1. 为什么需要 nftables：iptables 的四个结构性问题

| # | iptables 的问题 | nftables 的解法 | 源码对应 |
|---|---|---|---|
| 1 | 每个 match/target 是独立内核模块（`xt_tcp.o`、`xt_state.o`...） | 一套 VM + 少量内建表达式，模块只提供新**表达式类型** | `nft_register_expr()` |
| 2 | 规则 = `xt_entry` 链表，每条规则线性回调进 match/target | 规则**预编译成连续内存 blob**（见 §2） | `nft_rule_dp` |
| 3 | v4/v6 两套（iptables + ip6tables），规则漂移 | `NFPROTO_INET` 一套 | family 枚举 |
| 4 | 集合要外挂 ipset（另一个子系统、另一套命令） | set 是 nft 一等公民，三种后端自动选择 | `nft_select_set_ops()` |

**换来的运维优势**（同样重要）：原子替换整个规则集（`nft -f`，一瞬间新旧切换，无窗口期）、规则带注释/计数器/超时天然融合、set 支持动态增删（`nft add element` 不触碰规则）。

---

## 2. 求值引擎：`nft_do_chain()` 逐段解析

这是 nftables 的心脏（`net/netfilter/nf_tables_core.c`）。先看结构，再拆关键点。

```c
unsigned int
nft_do_chain(struct nft_pktinfo *pkt, void *priv)
{
	const struct nft_chain *chain = priv, *basechain = chain;
	...
	bool genbit = READ_ONCE(net->nft.gencursor);
	struct nft_rule_blob *blob;
	struct nft_jumpstack jumpstack[NFT_JUMP_STACK_SIZE];   /* ⭐ 16 层 */

do_chain:
	if (genbit)
		blob = rcu_dereference(chain->blob_gen_1);    /* ⭐ 双代际 blob */
	else
		blob = rcu_dereference(chain->blob_gen_0);

	rule = (struct nft_rule_dp *)blob->data;
next_rule:
	regs.verdict.code = NFT_CONTINUE;
	for (; !rule->is_last ; rule = nft_rule_next(rule)) {       /* 规则线性扫 */
		nft_rule_dp_for_each_expr(expr, last, rule) {           /* 表达式线性扫 */
			if (expr->ops == &nft_cmp_fast_ops)
				nft_cmp_fast_eval(expr, &regs);          /* ⭐ fast path 直调 */
			else if (expr->ops == &nft_cmp16_fast_ops)
				nft_cmp16_fast_eval(expr, &regs);
			else if (expr->ops == &nft_bitwise_fast_ops)
				nft_bitwise_fast_eval(expr, &regs);
			else if (expr->ops != &nft_payload_fast_ops ||
				 !nft_payload_fast_eval(expr, &regs, pkt))
				expr_call_ops_eval(expr, &regs, pkt);    /* 慢路径：间接调用 */

			if (regs.verdict.code != NFT_CONTINUE)
				break;
		}

		switch (regs.verdict.code) {
		case NFT_BREAK:            /* 本条规则没匹配上 */
			regs.verdict.code = NFT_CONTINUE;
			continue;          /* 下一条规则 */
		case NFT_CONTINUE:         /* 本条规则全部表达式通过 */
			continue;          /* 下一条规则（无 verdict = 继续） */
		}
		break;                     /* 有 verdict：跳出规则循环 */
	}
	...
}
```

### 2.1 规则是 blob，不是链表

每条链的规则被**预序列化成一段连续内存**（`nft_rule_blob` → `nft_rule_dp` 数组，`dp` = datapath）。指针运算（`nft_rule_next(rule)`）在连续内存上前进，对 CPU cache 极度友好——这是 nftables 相对 iptables `xt_table` 链表遍历的第一个性能来源。

**一条 nft 规则的结构**：

```
nft add rule inet filter input tcp dport 9090 ip saddr 10.0.0.0/24 accept
                 └──── payload ────┘└──── cmp ────┘└─── payload+cmp ───┘└─ verdict ─┘
```

`nft` 用户态工具把这条命令**编译成一串表达式**：`payload(L4头, dport字段) → cmp(==9090) → payload(L3头, saddr) → cmp(in 10.0.0.0/24) → verdict(accept)`。每个表达式自带 ops（函数表），数据紧跟在 ops 后面。**编译发生在用户态**，内核只收到「装好的表达式序列」——这是"VM"一词的由来。

### 2.2 fast path：四种表达式直调

```c
if (expr->ops == &nft_cmp_fast_ops)
	nft_cmp_fast_eval(expr, &regs);
```

不是通过 `expr->ops->eval()` 间接调用，而是**比对 ops 指针后直接调**。为什么？看文件开头的注释：

```c
#if defined(CONFIG_RETPOLINE) && defined(CONFIG_X86)
static struct static_key_false nf_tables_skip_direct_calls;
```

**Spectre v2 缓解（retpoline）让间接调用变贵**（数十 cycle 的陷阱序列），所以 v6.6 给最热的四种表达式做了直调旁路：

| fast path | 覆盖场景 | 优化点 |
|---|---|---|
| `nft_cmp_fast` | 32 位以内的相等比较（端口、协议号） | `((regs->data[sreg] & mask) == data) ^ inv` 一条指令组 |
| `nft_cmp16_fast` | 128 位比较（**IPv6 地址**） | 两个 u64 并行比较 |
| `nft_bitwise_fast` | 掩码异或（`and`/`mask` 类操作） | 一条 AND+XOR |
| `nft_payload_fast` | 从包头取字段 | 不走间接调用 |

**HFT 含义**：典型的「IP+端口白名单」规则（payload + cmp + payload + cmp + verdict）在 v6.6 上**几乎全部命中 fast path**，单条规则的求值成本在十几 cycle 量级。防火墙本身不再是借口——位置（hook 越晚越贵）才是。

### 2.3 verdict 的内部值

```c
/* include/uapi/linux/netfilter/nf_tables.h:65-69 */
NFT_CONTINUE = -1,   /* 规则内：继续下一个表达式；规则间：继续下一条规则 */
NFT_BREAK    = -2,   /* 本条规则匹配失败，试下一条 */
NFT_JUMP     = -3,   /* 跳到另一条链，记住回来的位置 */
NFT_GOTO     = -4,   /* 跳到另一条链，不回来 */
NFT_RETURN   = -5,   /* 提前返回跳转栈顶的链 */
```

注意这些是**负数**，与 Netfilter 层的正数 verdict（`NF_ACCEPT=1`...）错开。求值结束后统一转换：

```c
switch (regs.verdict.code & NF_VERDICT_MASK) {
case NF_ACCEPT:
case NF_DROP:
case NF_QUEUE:
case NF_STOLEN:
	return regs.verdict.code;      /* 交给 nf_hook_slow 解释 */
}
```

**`NFT_BREAK` 是「规则不匹配」的信号**：任何一个 cmp 表达式失败就把 verdict 设为 BREAK，表达式循环 break，规则循环 continue。理解了这一点，「nft 规则为什么天然线性扫描、无法跳过」就清楚了——**没有 rule skipping 优化，规则数就是成本**（大规则集请用 set）。

### 2.4 jump 栈：16 层上限

```c
struct nft_jumpstack jumpstack[NFT_JUMP_STACK_SIZE];   /* 16 */
...
case NFT_JUMP:
	if (WARN_ON_ONCE(stackptr >= NFT_JUMP_STACK_SIZE))
		return NF_DROP;          /* ⭐ 超过 16 层跳转 = 直接丢包 */
	jumpstack[stackptr].rule = nft_rule_next(rule);
	stackptr++;
	fallthrough;
case NFT_GOTO:
	chain = regs.verdict.chain;
	goto do_chain;
```

`jump`（带栈返回）vs `goto`（尾调用，不压栈）。**跳转深度超过 16 直接 NF_DROP**——不是拒绝加载，是运行时丢包。设计规则集时链嵌套别超过 16 层（实际上超过 4-5 层就该重构了）。

### 2.5 trace：排错的杀手锏

```c
if (static_branch_unlikely(&nft_trace_enabled))   /* 静态键，默认零开销 */
	nft_trace_init(&info, pkt, basechain);
```

`nft add rule ... trace` 之后，每个包在每条规则上的走向都会通过 netlink 多播出来，`nft monitor trace` 实时看「这个包为什么被丢」。**静态键保证不 trace 时零成本**——生产环境可以常备 trace 规则，不用时不开就没有开销。

---

## 3. 原子替换：双代际 blob

```c
bool genbit = READ_ONCE(net->nft.gencursor);    /* 进链时读一次代际位 */
...
if (genbit)
	blob = rcu_dereference(chain->blob_gen_1);  /* 新规则装在另一个 blob 里 */
else
	blob = rcu_dereference(chain->blob_gen_0);
```

每个链有**两个 blob 槽**（`blob_gen_0` / `blob_gen_1`）。更新规则集时：

1. 新规则序列化进**未在用的那个槽**
2. commit 时翻转 `gencursor`（`nf_tables_api.c:9948`：`net->nft.gencursor = nft_gencursor_next(net)`）
3. 正在旧 blob 上跑的包（RCU 读临界区）跑完自己的链，自然结束
4. grace period 后回收旧 blob

**这就是 `nft -f rules.conf` 的原子性来源**：不存在「一半新一半旧」的窗口。正在求值的包看到的是完全一致的旧规则集或新规则集。对比 iptables 逐条 COMMIT 的方式（中间态可被包看到），这是生产环境热更新规则的安全保证。

**代际位只在进链时读一次**（`READ_ONCE(net->nft.gencursor)`），链内跳转（jump/goto `do_chain`）不会重读——保证同一次求值全程用同一个代际，不会被中途翻转撕裂。

---

## 4. set：三种后端与自动选择

### 4.1 set 的 flags（`include/uapi/linux/netfilter/nf_tables.h`）

```c
enum nft_set_flags {
	NFT_SET_ANONYMOUS = 0x1,   /* 无名集合（编译期生成，随规则删除） */
	NFT_SET_CONSTANT  = 0x2,   /* 内容绑定后不可变 */
	NFT_SET_INTERVAL  = 0x4,   /* 包含区间（10.0.1.0/24 这种） */
	NFT_SET_MAP       = 0x8,   /* 字典：key → data（vmap 可到 verdict） */
	NFT_SET_TIMEOUT   = 0x10,  /* 元素带超时（自动过期） */
	NFT_SET_EVAL      = 0x20,  /* 可从求值路径更新（add/update/dump） */
	NFT_SET_OBJECT    = 0x40,  /* 元素是 stateful object（counter/limit/quota） */
	NFT_SET_CONCAT    = 0x80,  /* 拼接 key（ip+port 一起查） */
	NFT_SET_EXPR      = 0x100, /* 元素挂表达式 */
};
```

### 4.2 三后端能力表（v6.6 `.features` 字段核对）

| 后端 | 源文件 | 支持的 flags | 数据结构 | 擅长 |
|---|---|---|---|---|
| **rhash** | `nft_set_hash.c:729` | MAP\|OBJECT\|**TIMEOUT**\|EVAL | rhashtable（可动态扩容） | 大量离散 IP，动态增删 |
| **hash** / hash_fast | `nft_set_hash.c:753/772` | MAP\|OBJECT | 固定大小哈希表 | 元素数已知（`size N`），省内存 |
| **rbtree** | `nft_set_rbtree.c:757` | **INTERVAL**\|MAP\|OBJECT\|TIMEOUT | 红黑树 | 区间集合（CIDR 白名单） |
| **pipapo** / pipapo_avx2 | `nft_set_pipapo.c:2281/2305` | **INTERVAL**\|MAP\|OBJECT\|TIMEOUT | PIPAPO 词典 | **拼接 key**（IP+端口+协议一起匹配） |

（pipapo_avx2 仅 x86_64，用 AVX2 批量匹配，是 pipapo 的加速版。）

**PIPAPO 是 nftables 的独门武器**（5.6+）：传统上「src ip ∈ CIDR ∧ dst port ∈ 范围 ∧ proto ∈ 集合」这种多维匹配要么多条规则线性扫，要么拆成多个 set 再 AND。pipapo 把拼接 key（`NFT_SET_CONCAT`，字段数 >1 时强制，`nf_tables_api.c:4995`）按字段分别建词典，一次查表完成多维区间匹配——**这是 nftables 在「大规则集 + 多字段」场景下反超手写规则链的关键**。

### 4.3 选择算法：`nft_select_set_ops()`

```c
/* nf_tables_api.c —— 按声明特征自动选后端 */
static bool nft_set_ops_candidate(const struct nft_set_type *type, u32 flags)
{
	return (flags & type->features) == (flags & NFT_SET_FEATURES);
}

/* 遍历 nft_set_types[]（5 个注册项），estimate() 估成本，按 policy 取最优 */
switch (desc->policy) {
case NFT_SET_POL_PERFORMANCE:   /* 性能优先（默认） */
	if (est.lookup < best.lookup) break;      /* lookup 更快者胜 */
	...
case NFT_SET_POL_MEMORY:        /* 内存优先 */
	if (est.space < best.space) break;        /* 占用更小者胜 */
	...
}
```

**实操含义**：

- `type ipv4_addr` + 无 flags → hash 系（O(1)）
- `flags interval`（单字段）→ rbtree（O(log n)）
- 拼接字段（`concat`，等值匹配）→ hash 系（拼接 key 整体哈希，O(1)）
- 拼接字段 + `flags interval` → **pipapo**（其 estimate 只接受「interval + 字段数 ≥ 2」，见 Q4）

**HFT 白名单的选型路径**：交易所行情源 = 几十个 CIDR → `flags interval` → rbtree，查一次 O(log n)，n≈几十时 ~6 次比较，全部在 fast path 的 cmp 之外独立进行。比等价的手写规则链（每源 2-4 条规则线性扫）便宜一个数量级。

### 4.4 vmap：集合直接给 verdict

```bash
nft add map inet filter md_vmap '{ type ipv4_addr : verdict \; }'
nft add element inet filter md_vmap '{ 10.0.1.5 : accept, 10.0.9.9 : drop \; }'
nft add rule inet filter input ip saddr vmap @md_vmap
```

一条规则 + 一个 map = 完整的「不同来源不同处置」策略。比每个来源一条规则（规则数线性涨）便宜得多。

---

## 5. iptables-nft 兼容层：能跑，但有代价

iptables-nft 把 iptables 语法**翻译成 xt 兼容表达式**（`xt_match`/`xt_target` 包装）塞进 nft VM：

```
iptables-nft:  规则 → [xt_match 包装] → [xt_target 包装]    （间接调用，无 fast path）
原生 nft:      规则 → [payload] → [cmp] → [verdict]          （fast path 直调）
```

代价两层：

1. **性能**：xt 包装是普通表达式，走 `expr_call_ops_eval()` 间接调用，没有 §2.2 的直调旁路
2. **能力**：享受不到 set/vmap/timeout/trace 这些 nft 原生特性

**迁移策略**：先用 iptables-nft 平稳过渡（语义不变），然后 `iptables-translate` 逐条翻译 + 重写集合逻辑为原生 set，最后 `update-alternatives` 切走。**不要在性能敏感机器上长期跑兼容层。**

---

## 6. HFT 要点

1. **规则数 = 成本**（线性扫，无 skipping）。白名单用 set/vmap，别手写规则链。上千条规则的 filter 链，每个包都要全扫到 verdict 为止。
2. **fast path 覆盖了典型五元组匹配**：v6.6 的 cmp/cmp16/bitwise/payload 直调让「单条简单规则」的成本低到可以忽略——**位置（hook 深度）才是大头**。
3. **双代际 blob = 生产热更新安全**：改规则集不会有中间态窗口，敢在交易时段改防火墙。
4. **拼接维度多就上 pipapo**：`concat` + interval 的多维白名单是它的主场，一次查表完成。
5. **jump 栈只有 16 层，超了是丢包不是报错**——规则集设计时链嵌套 ≤ 5 层。
6. **trace 静态键默认零开销**，排障规则可以常备。

---

## 7. 与 Rosen Ch9 的差异

| 维度 | Rosen 3.x | v6.6 |
|---|---|---|
| 规则存储 | `xt_table` 链表 | 连续内存 blob（cache 友好） |
| 求值 | match/target 回调链 | VM 表达式序列 + fast path 直调 |
| 原子替换 | 逐条 commit（有中间态） | 双代际 blob 瞬时切换 |
| 集合 | ipset 外挂 | 原生三后端 + pipapo 多维匹配 |
| 追踪 | 无 | netlink trace（静态键，默认零开销） |

---

## 8. 代码自测

<details>
<summary>Q1：`nft add rule inet filter input tcp dport 9090 accept` 编译成哪些表达式？</summary>

1. `payload`：从 L4 头取 dport 字段（fast path）
2. `cmp`：== 9090（32 位以内 → `nft_cmp_fast_eval` 直调）
3. `verdict`：accept

执行序列：payload_fast → cmp_fast → verdict。**两个都走直调旁路**，无间接调用。
</details>

<details>
<summary>Q2：规则链里有 500 条 drop 规则 + 最后一条 accept。对白名单流量，求值成本是多少？</summary>

**501 条规则全扫**。白名单包要先跨过 500 条不匹配的 drop（每条 `NFT_BREAK` → 下一条），直到最后的 accept。

这是线性扫描的本性：**不匹配的规则也是成本**。500 条 drop 换成一个 `@blacklist` set + 一条 drop 规则，成本从 500 次「表达式序列求值」降为 1 次哈希/树查找。

（对比：如果 drop 的是端口等 cmp_fast 表达式，单条成本低，但 500 条累积依然显著——尤其在你以为「防火墙很便宜」的时候。）
</details>

<details>
<summary>Q3：`nft -f` 替换规则集的瞬间，正在被处理的包会看到什么？</summary>

看到**完整一致的旧规则集**。机制：

1. 包进链时 `READ_ONCE(net->nft.gencursor)` 读一次代际位（如当前是 gen_0）
2. 整条链的求值全程锁在 `blob_gen_0` 上（jump/goto 重进 `do_chain` 也不重读代际位）
3. commit 翻转 gencursor 到 gen_1 —— 只影响之后进链的包
4. 旧 blob 等 RCU grace period 后回收

不存在「半新半旧」。这也是为什么生产环境规则更新**必须**用 `nft -f ruleset` 全量替换而不是逐条 `nft add`（后者每条都是一次独立事务，中间态可被包看到）。
</details>

<details>
<summary>Q4：`nft add set x '{ type ipv4_addr . inet_service \; }'` 创建的 set 用的什么后端？如果加 `flags interval` 呢？</summary>

分两种情况（v6.6 源码验证）：

**无 interval（纯等值匹配）→ hash 系后端。** `ipv4_addr . inet_service` 是拼接类型（`desc.field_count > 1` 强制 `NFT_SET_CONCAT` flag，`nf_tables_api.c:4995`）。但注意 `NFT_SET_FEATURES` 掩码（`nf_tables_api.c:4158`）**不含 CONCAT**——后端候选判定只看 INTERVAL|MAP|TIMEOUT|OBJECT|EVAL。所以无 interval 的 concat set 是普通候选，rhash/hash 的 estimate 都通过，拼接 key 整体打包成一个哈希 key，**O(1)**。

**有 interval（区间 + 拼接）→ 只能 pipapo。** `nft_pipapo_estimate()`（`nft_set_pipapo.c:2079`）开头两行：

```c
if (!(features & NFT_SET_INTERVAL) ||
    desc->field_count < NFT_PIPAPO_MIN_FIELDS)   /* = 2 */
	return false;
```

pipapo 只接受「interval + 至少两个字段」的 set；而 rbtree 虽然支持 INTERVAL，但对多字段区间的表达能力不足（拼出来的大 key 之间无法做字段独立的范围匹配）。所以 `concat + interval` 的组合**实际收敛到 pipapo**（x86_64 上是 avx2 加速版）。

一句话：**等值拼接走哈希，区间拼接走 pipapo。**
</details>

<details>
<summary>Q5：为什么 nft 规则没有类似「按首字段建索引跳过不匹配规则」的优化？</summary>

设计取舍，不是疏忽：

1. **tc/iptables 时代教训**：优化规则查找的复杂结构（如 iptables 的 chain jump 编排）让语义变晦涩，排错困难
2. **nft 的答案是把「快」外包给 set**：多维快查用 pipapo/hash 后端（O(1)~O(log n)），规则链保持「顺序、可预测、可 trace」
3. blob 连续内存 + fast path 直调已经把**每条规则的常数**压得很低；规则数多到线性扫描成为瓶颈时，正确解法是 set 化，不是给规则链建索引

这也是 03 篇对比的伏笔：**BPF 是「把逻辑编译成任意程序」，nft 是「把逻辑约束成可优化的数据操作」**——两者哲学不同，各有主场。
</details>

---

## 导航

- **上一篇：** [01-nftables-bootlin.md](01-nftables-bootlin.md) — hook 体系、优先级、verdict 语义
- **下一篇：** [03-nftables-vs-bpf.md](03-nftables-vs-bpf.md) — nftables vs XDP/tc-BPF 选型
- **相关：** [chapter-09-tc-bpf/](../../chapter-09-tc-bpf/) tc-BPF 的 tcx 机制（同期的 hook 对比） · [chapter-11-packet-filter-flowtable/](../../chapter-11-packet-filter-flowtable/) flowtable：Netfilter 自己的快速路径
- **章节主页：** [README](../README.md)
