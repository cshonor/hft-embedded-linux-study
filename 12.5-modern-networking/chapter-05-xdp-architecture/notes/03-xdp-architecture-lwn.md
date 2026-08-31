# 03 — XDP 架构全景：五个动作、verifier 边界与定位

> **对应 Rosen:** 无（书出版时 XDP 不存在）
> **内核版本:** XDP hook 4.8+；AF_XDP 4.18+；本文以 **v6.6** 为准
> **源码:** `include/uapi/linux/bpf.h`、`include/uapi/linux/if_link.h`、`net/core/dev.c`

## 文档概述

本篇是 chapter-05 的**总纲**：XDP 在内核里到底挂在哪、能做什么、不能做什么。

本篇与兄弟篇的分工：

| 篇 | 讲什么 |
|----|--------|
| [01 XDP 实操](01-xdp-bootlin.md) | 工具链、模式选择、加载失败排查、实验 |
| [02 AF_XDP 四 ring](02-xdp-rings.md) | AF_XDP 的 UMEM 与无锁同步 |
| **03（本篇）** | **架构总纲**：五个动作、四种模式、**verifier 能力边界**、use case、与 DPDK 定位 |
| [chapter-07](../../chapter-07-xdp-redirect-dpdk/) | XDP vs DPDK 的**详细对比**（本篇只给结论表） |

原笔记 2.5 KB，讲了"XDP 是什么 + 四种模式 + 一张 use case 表 + XDP vs DPDK"。
本篇保留这些并补上两块原笔记完全没有的内容：

1. **Generic 模式什么时候会退化成"拷贝"**（源码里有三个明确条件）
2. **verifier 的能力边界**——XDP 程序能做什么、不能做什么

---

## 一、XDP 的精确定位

XDP 的卖点常被说成"快"，但**精确的定位**是：

> **在 `sk_buff` 分配之前、在协议栈看到包之前，给一个可编程的决策点。**

```
驱动 Rx poll
  ├─ 从 Rx ring 取描述符
  ├─ 拿 page_pool 的页
  ├─ 构造 xdp_buff（栈上，8 个字段，零分配）
  ├─ ★ 跑 XDP 程序  ← 就在这里
  │    返回动作决定包的一生
  ├─ XDP_PASS → napi_build_skb()（skb 在这里才诞生）
  └─ 其他动作 → skb 从未存在
```

**这个位置带来两个后果**，是理解 XDP 全部价值的钥匙：

| 后果 | 含义 |
|------|------|
| **省掉 skb 分配** | 100–200 ns，高 PPS 下是实打实的 |
| **省掉整条协议栈** | GRO、ptype_all、tc ingress、Netfilter、路由、L4、socket 队列——**全省** |

**注意"省"的顺序**：`XDP_DROP` 不是"做了某件事很快"，而是**什么都没做**。
它连 skb 都没生成，于是也没有"释放 skb"这一步——页面直接从 XDP 层还回 page_pool。

### ⚠️ 只有 native/offloaded 才在这个位置

**Generic 模式不是**。它的挂载点在 `__netif_receive_skb_core()`（dev.c:5373）里，
那时 skb 早就分配好了。Generic 模式的价值是**功能验证**，不是性能。

不过有一个一致的推论：generic XDP 在 `ptype_all`（dev.c:5394）**之前**，
所以 generic 模式的 `XDP_DROP` 包 **tcpdump 也看不到**——与 native 模式行为一致。

---

## 二、五个动作（`enum xdp_action`）

```c
/* include/uapi/linux/bpf.h */
enum xdp_action {
	XDP_ABORTED = 0,
	XDP_DROP,
	XDP_PASS,
	XDP_TX,
	XDP_REDIRECT,
};
```

| 动作 | 值 | 语义 | 包去哪了 | 页面归属 |
|------|-----|------|---------|---------|
| **`XDP_ABORTED`** | **0** | **程序出错**，丢弃并记 tracepoint | 丢弃 | 回收给 page_pool |
| `XDP_DROP` | 1 | 主动丢弃 | 丢弃 | 回收给 page_pool |
| `XDP_PASS` | 2 | 放行，交给协议栈 | 建 skb → 协议栈 | 页变成 skb 数据区（零拷贝） |
| `XDP_TX` | 3 | 原路反弹（从收包的网口发回去） | 发回同一网口 | 复用，发完回收 |
| `XDP_REDIRECT` | 4 | 重定向到别处 | AF_XDP / cpumap / devmap / veth | 随 `xdp_frame` 走 |

### ⚠️ 陷阱：`XDP_ABORTED = 0`

**返回 0 不是"放行"，是"出错丢弃"**。

C 语言里 0 通常表示"无动作/成功"，但 XDP 选择了相反的语义。所以：

- 忘了写 return 的路径 → 返回 0 → `XDP_ABORTED` → **静默吃掉所有包**
- 显式 `return 0` 想表示放行 → 实际是丢弃

这个坑的隐蔽之处在于：**verifier 通过、程序加载成功、看起来一切正常，
只是所有包都不见了。**

```bash
# 检查
bpftrace -e 'tracepoint:xdp:xdp_exception { @[args->act] = count(); }'
#   ABORTED 计数在涨 → 有路径返回了 0
```

→ 详见 [01 篇 Q2](01-xdp-bootlin.md)。

### `XDP_REDIRECT` 的四个目标

| 目标 | 用途 | HFT 相关 |
|------|------|---------|
| **AF_XDP socket** | 送到用户态（零拷贝） | ★ 内核旁路收行情 |
| **cpumap** | 转到另一个 CPU 上处理 | ⚠️ 跨核有代价，见 [chapter-03/04](../../chapter-03-tx-path-skbbuff/notes/04-sk-buff-xdp-buff.md) |
| **devmap** | 转发到另一个网口 | 软件交换机 |
| **veth / 其他设备** | 容器网络 | — |

⚠️ **`XDP_REDIRECT` 时需要把 `xdp_buff` 转成 `xdp_frame`**，而
`xdp_frame` **就写在包自己的 headroom 里**（`xdp_convert_buff_to_frame()`）。
这也是 XDP 强制要求 headroom 的硬性理由。
→ 详见 [chapter-03/04](../../chapter-03-tx-path-skbbuff/notes/04-sk-buff-xdp-buff.md)。

---

## 三、四种模式与 `XDP_FLAGS`

```c
/* include/uapi/linux/if_link.h:1295 */
#define XDP_FLAGS_UPDATE_IF_NOEXIST	(1U << 0)
#define XDP_FLAGS_SKB_MODE		(1U << 1)
#define XDP_FLAGS_DRV_MODE		(1U << 2)
#define XDP_FLAGS_HW_MODE		(1U << 3)
#define XDP_FLAGS_REPLACE		(1U << 4)
```

| 模式 | flag | 挂载点 | skb 分配了吗 | 何时用 |
|------|------|--------|-------------|--------|
| **Native (DRV)** | `DRV_MODE` | 驱动 Rx poll | ❌ | **默认目标** |
| **Generic (SKB)** | `SKB_MODE` | `__netif_receive_skb_core()` dev.c:5373 | ✅ 已分配 | 驱动不支持时；功能验证 |
| **Offloaded (HW)** | `HW_MODE` | 网卡硬件 | ❌ 不进主机 | 仅少数 SmartNIC |
| — | `UPDATE_IF_NOEXIST` | 仅在没有程序时才挂载 | — | 防止覆盖 |
| — | `REPLACE` | 替换已挂载的程序 | — | 热更新（配合 `XDP_FLAGS_MODES` 指定模式） |

### ⚠️ Generic 模式的三个"补拷"条件（源码原文）

这是本篇最技术性的一个点。Generic 模式下包已经是 skb 了，而 XDP 程序
**要求包是线性的、且有足够 headroom**。内核源码（dev.c:4950-4965）里写着：

```c
	if (skb_is_redirected(skb))
		return XDP_PASS;

	/* XDP packets must be linear and must have sufficient headroom
	 * of XDP_PACKET_HEADROOM bytes. This is the guarantee that also
	 * native XDP provides, thus we need to do it here as well.
	 */
	if (skb_cloned(skb) || skb_is_nonlinear(skb) ||
	    skb_headroom(skb) < XDP_PACKET_HEADROOM) {
		int hroom = XDP_PACKET_HEADROOM - skb_headroom(skb);
		int troom = skb->tail + skb->data_len - skb->end;
		...
		if (pskb_expand_head(skb, ...))
```

翻成判定表——**满足任一条件，generic XDP 就会先拷贝/线性化一份再跑程序**：

| 条件 | 什么时候会命中 |
|------|--------------|
| `skb_cloned(skb)` | 包被克隆过（**tcpdump 正在抓包时几乎必然命中**） |
| `skb_is_nonlinear(skb)` | 有非线性区（GRO 合并后、大包分片） |
| `skb_headroom(skb) < XDP_PACKET_HEADROOM` | headroom 不足 256 字节 |

**这一条解释了一个很反直觉的现象**：generic 模式下，**开着 tcpdump 反而更慢**——
因为 tcpdump 让 skb 被 clone，于是每个包都要多一次 `pskb_expand_head()` 拷贝。

（对照 [chapter-04/02](../../chapter-04-page-pool/notes/02-page-pool-lwn.md) 的陷阱 2：
tcpdump 会抬高 refcnt 让 page_pool 的页回不了池。**同一个动作，两处惩罚。**）

还有一行值得注意：`if (skb_is_redirected(skb)) return XDP_PASS;`——
**已经在别处被 redirect 过的包，不会再跑一遍 generic XDP**，避免重复处理。

---

## 四、verifier：XDP 程序的能力边界

**verifier 是 XDP 相对于内核模块的核心优势**：它把"运行时崩溃"变成"加载时拒绝"。
代价是你必须按它的规则写。

### 必须遵守的三条硬规则

**① 逐层边界检查（最常见的新手障碍）**

包数据在解引用**之前**必须先验证指针范围：

```c
void *data = (void *)(long)ctx->data;
void *data_end = (void *)(long)ctx->data_end;

struct ethhdr *eth = data;
if ((void *)(eth + 1) > data_end)      /* ① 先验证 */
    return XDP_DROP;
if (eth->h_proto != htons(ETH_P_IP))   /* ② 再解引用 */
    return XDP_PASS;

struct iphdr *ip = (void *)(eth + 1);
if ((void *)(ip + 1) > data_end)       /* 每一层都要 */
    return XDP_DROP;
/* UDP 同理 */
```

**每一层都要检查一遍**，verifier 才会放行。漏一层 = 加载被拒。

**② 不能有无界循环**

verifier 必须能证明程序会终止。传统写法只能用有界循环（`#pragma unwind` 展开），
较新内核提供了 `bpf_loop()` helper 和 open-coded iterators。**不要在 XDP 程序里
写 `while(1)` 或依赖运行时条件的循环**。

**③ 栈只有 512 字节**

局部变量要精简。需要大缓冲就放 map。

### 能做什么 / 不能做什么

| ✅ 能做 | ❌ 不能做 |
|--------|----------|
| 读写包内容（bounds-checked 后） | 随机访问未验证的包数据 |
| `bpf_xdp_adjust_head()` / `adjust_tail()`（增删封装头） | 无界循环、递归 |
| 查/改 map（计数、统计、流表） | 阻塞操作（睡眠、加锁等长时间等待） |
| `bpf_ktime_get_ns()` 打时间戳 | 访问用户态内存 |
| `bpf_redirect()` / `bpf_redirect_map()` | 调用任意内核函数（只能调白名单 helper） |
| `bpf_xdp_event_output()` 采样送用户态 | 分配内存（没有 malloc） |
| 触发 `bpf_printk()`（调试用，**很慢**） | — |

⚠️ **`bpf_printk()` 只能用于调试**：它走 trace pipe，每个包调用会彻底毁掉性能。
生产程序用 map 计数，或 `bpf_perf_event_output()` 采样。

### verifier 报错怎么读

```bash
bpftool prog load xdp_prog.o /sys/fs/bpf/p 2>&1 | tail -40
```

v6.6 的 verifier 日志会给出：
- 出错的**指令序号**
- 出错时各寄存器的**类型与取值范围**（`R1=pkt(offs=14,r=34)` 之类）
- **出错原因**（`invalid access to packet, off=34 size=2, R1(id=0,off=34,r=34)`）

配合 `llvm-objdump -S xdp_prog.o` 把指令序号映射回 C 源码行。

---

## 五、use case 全景

| 场景 | 动作 | HFT 关联 |
|------|------|---------|
| DDoS 防护 | `XDP_DROP` 攻击包 | 保护交易服务器 |
| 行情早过滤 | `XDP_DROP` 无关组播组 | ★ 只放行目标行情，垃圾包不建 skb |
| **延迟测量** | 打时间戳进 map | ★ **HFT 最有用的用法**：分段测量 NIC→用户态 |
| 负载均衡 | `XDP_REDIRECT` 到后端 | 行情分发 |
| 协议预处理 | `XDP_TX` 快速响应 | ARP/ICMP 快速应答，不进协议栈 |
| 监控统计 | map 计数 | 丢包/延迟监控，几乎零成本 |
| **AF_XDP 收包** | `XDP_REDIRECT` 到 socket | ★ 内核旁路，接近 DPDK |
| 采样抓包 | `bpf_xdp_event_output()` | 比 tcpdump 开销低得多 |

### 对 HFT 最有价值的两个用法

**① 延迟分段测量**

在 XDP 程序里 `bpf_ktime_get_ns()` 打时间戳存进 map，用户态收到包时再取一次
`clock_gettime()`，两者相减 = **NIC → 用户态的端到端延迟**。这是唯一能在不改动
内核、不换网卡的前提下拿到"收包路径真实耗时"的方法。

**② 行情早过滤**

行情是多路组播，你只关心其中几路。在 XDP 层按组播地址/端口过滤，
**无关的包连 skb 都不建**。这比在 socket 层过滤省一个量级。

---

## 六、XDP vs DPDK：结论表

（详细对比见 [chapter-07](../../chapter-07-xdp-redirect-dpdk/)，这里只给结论。）

| 维度 | XDP | DPDK |
|------|-----|------|
| 运行位置 | 内核态（但极早） | 用户态（完全旁路） |
| **网卡归属** | **仍归内核**（可与内核功能共存） | **被独占**（绑定 UIO/VFIO，内核看不到这个口） |
| sk_buff | 可以不建 | 从不建 |
| 部署复杂度 | 低（加载 BPF 程序） | 高（绑驱动、大页、NUMA） |
| 灵活性 | 高（可与其他内核功能共存） | 低 |
| 极限延迟 | 略高 | 最低 |
| **HFT 定位** | **中低频、需要内核功能、或渐进改造** | **超低延迟、co-location** |

**一句话**：XDP 和 DPDK 的真正区别不是"谁更快"，而是**谁拥有网卡**。
DPDK 独占网卡换来最低延迟；XDP 保留内核所有权，换来共存和灵活性。

---

## 七、性能怎么测（别信网上的绝对值）

```bash
# 最直接：bpftool 给出程序执行次数与总耗时
bpftool prog show id <id>
#   run_cnt  = 执行次数
#   run_time = 总耗时（ns）
#   → run_time / run_cnt = 单包平均耗时

# 对比不同模式：同一程序分别 native / generic 加载，各测一次
xdp-loader load --mode native  eth0 prog.o
bpftool prog show id <id>      # 记下 run_time/run_cnt
xdp-loader unload eth0 --all
xdp-loader load --mode generic eth0 prog.o
bpftool prog show id <id>      # 再记一次

# 端到端（HFT 真正关心的）：XDP 时间戳 vs 用户态收包时刻
```

⚠️ **不要在 veth 上测性能**——没有 DMA、没有真实 Rx ring，数字没有参考价值。
→ 见 [01 篇 Q3](01-xdp-bootlin.md)。

---

## HFT 要点

- **XDP 的价值来自"在 skb 之前"这个位置**，不是"它是 eBPF 所以快"。Generic 模式不在这个位置，所以不省 skb。
- **`XDP_ABORTED = 0`**：返回 0 是出错丢弃，不是放行。忘了写 return 会静默吃掉所有包。
- **Generic 模式在 tcpdump 开启时会更慢**：`skb_cloned()` 命中 → 每个包多一次 `pskb_expand_head()` 拷贝。和 page_pool 的 refcnt 惩罚是同一个动作的两处后果。
- **verifier 是优势不是障碍**：它把运行时崩溃变成加载时拒绝。代价是逐层边界检查。
- **对 HFT 最有价值的用法是延迟分段测量**，不是包过滤。
- **XDP vs DPDK 的真正分界是谁拥有网卡**，不是谁更快。
- **生产程序别用 `bpf_printk()`**，用 map 计数或 perf event 采样。
- **每次加载后确认实际模式**（`xdp-loader status`），静默降级会毁掉性能结论。

## 与 Rosen 3.x 的差异

Rosen 写作时 XDP 不存在。从**网络编程方法论**看，XDP 带来的是一次范式转移：

| Rosen 3.x 时代 | XDP 时代 |
|---------------|---------|
| 要改内核网络行为 → 写内核模块 | 写 eBPF 程序，verifier 保证安全 |
| 出问题 = oops / panic / 机器挂 | 出问题 = **加载被拒**（编译期拦截） |
| 包处理只能在协议栈内 | 包处理在**协议栈之前** |
| 用户态高性能收包 = `PF_PACKET` / `recvmmsg` | AF_XDP 零拷贝 |
| 观测靠 printk / 计数器 | map + tracepoint + `bpftool` |
| 功能演进要等内核发布 | **功能可以热加载**（`XDP_FLAGS_REPLACE`） |

最后一行是运维层面的重大变化：**XDP 程序可以在不重启、不重载驱动的情况下
热替换**（`XDP_FLAGS_REPLACE`）。对 HFT 这种不能随便停机重启的系统，这个能力的价值
不亚于性能本身。

---

## 代码自测

<details>
<summary>Q1：你要在 XDP 里做行情组播过滤，只放行 <code>239.1.2.3:12345</code>。写出最小可用代码，并说明每一处边界检查的必要性。</summary>

<b>答：</b>

```c
SEC("xdp")
int mcast_filter(struct xdp_md *ctx)
{
	void *data = (void *)(long)ctx->data;
	void *data_end = (void *)(long)ctx->data_end;

	struct ethhdr *eth = data;
	if ((void *)(eth + 1) > data_end)          /* ① 必须有 */
		return XDP_DROP;
	if (eth->h_proto != htons(ETH_P_IP))
		return XDP_DROP;                        /* ② 非 IP 直接丢 */

	struct iphdr *ip = (void *)(eth + 1);
	if ((void *)(ip + 1) > data_end)           /* ③ 必须有 */
		return XDP_DROP;
	if (ip->protocol != IPPROTO_UDP)
		return XDP_DROP;

	/* IP 头可能带选项，长度要用 ihl 算 */
	if (ip->ihl < 5)
		return XDP_DROP;
	struct udphdr *udp = (void *)ip + (ip->ihl * 4);
	if ((void *)(udp + 1) > data_end)          /* ④ 必须有 */
		return XDP_DROP;

	if (udp->dest != htons(12345))
		return XDP_DROP;                        /* ⑤ 端口过滤 */

	return XDP_PASS;
}
```

<b>为什么 ①③④ 必须有</b>：verifier 在<b>加载时</b>静态分析每一条路径。
它要知道"解引用 `eth + 1` 时，这个地址一定在 `[data, data_end)` 之内"。
没有那个 if，verifier 无法证明，<b>直接拒绝加载</b>——不是运行时崩溃，是根本装不上。

<b>容易漏的两点</b>：
1. **IP 头长度要用 `ip->ihl * 4`**，不能直接 `(void*)ip + 20`——
   带 IP 选项的包头部更长，硬编码 20 会读错位置。
2. **组播的 MAC 也要查**（更省）：组播 IP 会映射到组播 MAC
   （`01:00:5e:xx:xx:xx`），在 ① 之后立刻查 MAC 就能丢掉绝大部分无关流量，
   连 IP 头都不用解析。

<b>性能提示</b>：把过滤条件<b>按"能最早排除最多包"排序</b>。
MAC → 端口 → 才是完整解析。
</details>

<details>
<summary>Q2：你在 generic 模式做性能对比，发现开着 tcpdump 时吞吐明显下降，关掉就恢复。不是应该"tcpdump 只影响被抓的包"吗？</summary>

<b>答：</b>不对，tcpdump 会影响<b>所有</b>包——在 generic 模式下尤其明显。

看内核源码 dev.c:4950 附近：

```c
/* XDP packets must be linear and must have sufficient headroom
 * of XDP_PACKET_HEADROOM bytes. This is the guarantee that also
 * native XDP provides, thus we need to do it here as well.
 */
if (skb_cloned(skb) || skb_is_nonlinear(skb) ||
    skb_headroom(skb) < XDP_PACKET_HEADROOM) {
	...
	if (pskb_expand_head(skb, ...))
```

AF_PACKET（tcpdump 的底层）会给匹配的包<b>克隆一份</b>给抓包 socket，
于是 `skb_cloned(skb)` 为真 → 命中这个条件 → 每个包都要先
`pskb_expand_head()` **拷贝/线性化一次**，然后才轮到你的 XDP 程序跑。

所以 tcpdump 的成本不是"复制一份给抓包进程"，而是<b>让每个包都多了一次完整拷贝</b>。

<b>雪上加霜的是第二处惩罚</b>：clone 会抬高 page 的 refcnt，
按 page_pool 的规则（refcnt > 1 就真释放），这些页面<b>回不了池</b>——
池被抽干后每个包都要走 `alloc_page()` 慢路径。
→ 见 [chapter-04/02 陷阱 2](../../chapter-04-page-pool/notes/02-page-pool-lwn.md)

<b>实践建议</b>：
- 排障时尽量用 XDP 层采样（`bpf_xdp_event_output()`），而不是 skb 层全量抓包
- 必须抓包就加严格 filter，减少 clone 数量
- 抓完确认指标恢复——池需要时间重新填满
</details>

<details>
<summary>Q3：为什么 XDP 程序不能用无界循环？这给"在 XDP 里做复杂解析"带来什么限制？</summary>

<b>答：</b>因为 <b>verifier 必须在加载时静态证明程序一定会终止</b>。

verifier 不会运行你的程序，它做的是静态分析——遍历所有可能的执行路径，
证明每条路径都能在有限步内到达 return。无界循环（`while(1)`、
或退出条件依赖运行时数据的循环）无法静态证明终止，所以被拒绝。

<b>这带来三个实际限制</b>：

1. **不能写依赖包内容的循环**。比如"遍历所有 IP 选项直到遇到 EOL"——
   退出条件来自包数据，verifier 无法证明有界。**要写成固定次数的有界循环**。
2. **较新内核的解法**：用 `bpf_loop()` helper（把循环次数作为参数传给 helper，
   verifier 能验证），或用 open-coded iterators。老内核只能用
   `#pragma unroll` 完全展开。
3. **复杂解析要拆成 tail call 或用 map 存中间状态**。

<b>对 HFT 的含义</b>：XDP 适合做<b>固定格式的浅层解析</b>（MAC / IP / UDP 头、
固定偏移的行情协议字段）。如果你的协议需要变长遍历（TLV、变长字段），
XDP 层做起来会很痛苦——**这时候更应该把包 REDIRECT 到用户态，
在用户态用正常代码解析**。

<b>选型结论</b>：XDP 的强项是"用极低成本做粗筛和统计"，不是"做完整的协议解析"。
把它定位成<b>分流器</b>而不是<b>解析器</b>，设计会顺畅很多。
</details>

---

→ 前一篇：[02 AF_XDP 四 ring](02-xdp-rings.md)
→ 本章完，下一章：[chapter-06 AF_XDP](../../chapter-06-af-xdp/)
→ 相关：[chapter-01 hook 顺序](../../chapter-01-net-stack-architecture/notes/01-net-stack-architecture.md) · [chapter-04 page_pool](../../chapter-04-page-pool/) · [chapter-07 XDP vs DPDK](../../chapter-07-xdp-redirect-dpdk/)
