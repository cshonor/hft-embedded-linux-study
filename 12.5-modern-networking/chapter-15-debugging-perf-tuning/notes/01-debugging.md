# 15-01 — 网络调试工具链：丢包定位与延迟分解（v6.6 核验版）

> **Bootlin 课程模块：** Network Debugging Tools
> **对应 Rosen:** 无
> **内核源码路径:** `include/trace/events/skb.h:24`（kfree_skb + drop reason）、`include/trace/events/net.h`（net_dev_queue / netif_receive_skb）、`net/core/dev.c:4887`（bpf_prog_run_xdp）

## 章节导航

| 上一篇 | 本篇 | 下一篇 |
|---|---|---|
| [chapter-14 TCP/UDP 内部](../../chapter-14-tcp-udp-internals/README.md) | **15-01 调试工具链** | [15-02 性能调优](02-perf-tuning.md) |

## 本节讲什么

调试网络性能问题只有两类：**东西丢了**（丢在哪一层）和**东西慢了**（慢在哪一跳）。本篇按"计数器 → tracepoint → 函数级"三级递进组织工具链，并把旧笔记里两处 tracepoint 字段错误修正掉（`args->skb` 不存在，实际字段是 `args->skbaddr`；v6.6 丢包定位首选 `kfree_skb` 的 drop reason 而不是老 dropwatch）。

## 要点（先记住结论）

1. **先计数器后 tracepoint**：`ethtool -S`（驱动层）→ `/proc/net/softnet_stat`（softirq 层）→ `nstat`（协议层）→ `ss`（socket 层）——四层计数器定位丢包的"层"，零开销，先跑这些再上重武器。
2. **v6.6 丢包定位的正确姿势是 `kfree_skb` tracepoint + drop reason**：事件携带 `reason` 字段（`enum skb_drop_reason`，trace_skb.h:24-45，`__print_symbolic` 直接输出符号名）——5.17+ 起 300+ 处 `kfree_skb_reason()` 调用点都会给出结构化原因，dropwatch 只是它的老封装。
3. **`netif_receive_skb` / `net_dev_queue` 的 bpftrace 字段是 `args->skbaddr`**（net.h:23，`net_dev_template` 事件类）——旧笔记的 `args->skb` 是编造的，直接报错。
4. **XDP 程序计时 kprobe 的目标是 `bpf_prog_run_xdp()`**（dev.c:4887）——不是旧笔记写的 `xdp_prog_run`（内核里没有这个符号）。
5. **tcpdump 本身会改路径**：AF_PACKET 在 `ptype_all`（dev.c:5394），先于 tc ingress 执行（chapter-11-01 已详述）——测延迟时捕包工具不在被测路径上（用镜像口或硬件 tap）。
6. **诊断顺序即成本顺序**：每下一级工具，观测开销和定位精度同时上升——计数器（0 开销）→ tracepoint（每事件 ~1μs）→ kprobe + perf record（函数级但有 skid）。

## 一、四层计数器：零成本先跑

| 层 | 命令 | 看什么 |
|---|---|---|
| 驱动/网卡 | `ethtool -S eth0` | `rx_dropped`（驱动丢弃）、`rx_missed`（FIFO 溢出=带宽超限）、`rx_no_dma`/`rx_buf_alloc`（内存压力）；**per-queue 细分**能直接看出哪个队列打满 |
| 设备/softirq | `cat /proc/net/softnet_stat` | 第 1 列处理包数、第 2 列 **drop（backlog 满，`enqueue_to_backlog` 的 `netdev_max_backlog` 超限——见 [13-01](../../chapter-13-zerocopy-highperf/notes/01-scaling.md)）**、第 3 列 squeeze（软中断时间片耗尽分批） |
| 协议栈 | `nstat -az` | `UdpInErrors`/`UdpRcvbufErrors`（socket buffer 满）、`UdpInDatagrams`（对不上=路上丢）、`TcpExtListenDrops`（accept 队列满）、`TcpRetransSegs` |
| socket | `ss -tiuma` | `retrans`、`rtt`/`min/max`、`cwnd`、`pacing_rate`（EDT 模型的实际速率）、`skmem`（内存水位） |

对照法：**`ethtool -S` 的 rx 计数和 `nstat` 的 InDatagrams 对不上，差值就是"进了内核没到协议栈"**（GRO 合并、XDP 丢弃、backlog 丢）；协议栈计数和 `ss` 收到的对不上，就是 socket 层问题（buffer 满、filter 丢弃）。

## 二、丢包定位：kfree_skb 的 drop reason（v6.6 标准姿势）

tracepoint 定义（trace_skb.h:24，已核验）：

```c
TRACE_EVENT(kfree_skb,
	TP_PROTO(struct sk_buff *skb, void *location, enum skb_drop_reason reason),
	TP_STRUCT__entry(
		__field(void *,		skbaddr)
		__field(void *,		location)      // ← 丢弃现场的函数地址（%pS 符号化）
		__field(unsigned short,	protocol)
		__field(enum skb_drop_reason,	reason)  // ← 结构化原因
	),
	TP_printk("skbaddr=%p protocol=%u location=%pS reason: %s", ...
```

### perf 一条龙

```bash
# 抓 10 秒所有丢包，带原因和位置
perf record -e skb:kfree_skb -a sleep 10
perf script                       # 每行: 进程/CPU/时间 + location=pS + reason=SKB_DROP_REASON_XXX

# 只统计原因分布
perf record -e skb:kfree_skb -a sleep 10
perf script | awk -F'reason: ' '{print $2}' | sort | uniq -c | sort -rn
```

### bpftrace 按原因+位置聚合（开销最低的常驻监控）

```bash
bpftrace -e '
tracepoint:skb:kfree_skb {
	@[args->reason, kstack] = count();     // reason 枚举值 + 调用栈
}'
```

dropwatch（`-l kas` 模式）输出的"位置"与上面 `location=%pS` 同源——它就是 kfree_skb tracepoint 的 curses 前端；v6.6 上直接用 perf/bpftrace 拿到的信息严格更多（多了 reason）。

## 三、延迟分解：从 tracepoint 到 kprobe

### 收发路径 tracepoint（字段已核验：`skbaddr`）

```bash
# 发送侧：进 qdisc 的时刻（net.h:144，net_dev_template 类）
bpftrace -e 'tracepoint:net:net_dev_queue { @[args->name] = count(); }'

# 接收侧：进协议栈的时刻（net.h:151）
bpftrace -e 'tracepoint:net:netif_receive_skb { @[args->name] = count(); }'

# ⚠️ 旧笔记的 args->skb / @start[args->skb] 是错的：
# net_dev_template 的字段是 skbaddr/len/name（net.h:22-33），没有 skb 指针
```

### XDP 程序执行计时

```bash
# bpf_prog_run_xdp 是真实入口（dev.c:4887，native 与 generic 路径都过这里）
bpftrace -e '
kprobe:bpf_prog_run_xdp { @start[tid] = nsecs; }
kretprobe:bpf_prog_run_xdp /@start[tid]/ {
	printf("XDP: %d ns\n", nsecs - @start[tid]);
	delete(@start[tid]);
}'
```

### RX 完整延迟的正确量法

tracepoint 只能覆盖协议栈内部；**NIC 硬件时间戳 → 应用收到**的端到端延迟用 `SO_TIMESTAMPING`（硬件时间戳模式）——这正是本模块 [15-03 延迟量测](03-latency-measurement.md) 的主题，工具链上不要试图用 kprobe 拼端到端时间线（时钟域和精度都不对）。

### perf CPU 热点

```bash
perf record -g -e cycles -C <收包核> sleep 5     # 指定核，避免全核噪声
perf report --no-children                          # 自顶向下看热点函数
# 网络负载下高频出现在: netif_receive_skb* / tcp_v4_rcv / ip_rcv / napi poll 回调
```

## 四、tcpdump 的观测者效应（回顾）

chapter-11-01 的结论在此复核一遍：AF_PACKET 挂在 `ptype_all`（dev.c:5394），**早于 tc ingress 与所有协议栈处理**——每个包多一次 clone + BPF 过滤执行。生产延迟敏感路径上的排障流程：先用计数器锁定嫌疑层 → 停流量或镜像口上抓包。内核侧预过滤（`SO_ATTACH_FILTER`，cBPF→eBPF 翻译）能省用户态拷贝，但 clone 本身省不掉。

## 五、HFT 延迟诊断流程（修订版）

```
① 计数器定位层（零开销，随时跑）
   ethtool -S → 网卡/FIFO 丢包？per-queue 是否偏斜？
   /proc/net/softnet_stat → backlog 丢？squeeze 频繁？
   nstat -az → 协议层丢？重传率？
   ss -ti → rtt/cwnd/pacing_rate/retrans 异常？
        │
② 丢包已确认 → 结构化定位（低开销）
   perf record -e skb:kfree_skb → 按 reason + location 分组
        │
③ 瓶颈在处理慢而非丢 → 热点分析
   perf record -e cycles -g -C <核> → 函数级热点
        │
④ 怀疑 XDP/驱动层 → kprobe 计时
   bpf_prog_run_xdp 进出计时；驱动 poll 周期统计
        │
⑤ 端到端延迟分位数 → SO_TIMESTAMPING 硬件时间戳（15-03）
```

## 衔接

诊断讲完"哪里丢、哪里慢"。下一篇 [15-02 性能调优](02-perf-tuning.md) 给出修复侧：中断合并、offload 取舍（与 ch14 的纠错统一）、sysctl、qdisc、CPU/NUMA 隔离的完整清单——以及每项设置"为什么"的可辩护理由。

## 代码自测

<details>
<summary>Q1：为什么 v6.6 上 perf -e skb:kfree_skb 比 dropwatch 更好？</summary>

同源但信息更多：dropwatch 监听的就是 kfree_skb tracepoint（输出 location），而 v6.6 的事件多了 `reason` 字段（`enum skb_drop_reason`，300+ 处 `kfree_skb_reason()` 调用点填充）——dropwatch 的终端 UI 丢弃了这个字段。perf/bpftrace 能同时拿到 location（丢弃函数）和 reason（结构化原因如 `SKB_DROP_REASON_SOCKET_RCVBUFF`），还能直接做聚合统计。
</details>

<details>
<summary>Q2：ethtool -S 里 rx_missed 和 /proc/net/softnet_stat 第 2 列 drop 分别说明什么？</summary>

不同层：`rx_missed` 是网卡 **RX FIFO/描述符环**溢出——包根本没进主机（DMA 都没来得及），通常是带宽超限或驱动分配不及时；softnet_stat 的 drop 是 `enqueue_to_backlog` 里 **backlog 队列满**（超过 `netdev_max_backlog`，默认 1000）——包进了主机但软中断处理不过来。前者加钱换网卡/分流，后者调 backlog、开 RSS 分散 softirq 或找 CPU 瓶颈。
</details>

<details>
<summary>Q3：bpftrace 脚本里写 args->skb 为什么报错？正确字段是什么？</summary>

tracepoint 的可访问字段由 `TP_STRUCT__entry` 定义，不是 C 函数原型。`netif_receive_skb`/`net_dev_queue` 用的是 `net_dev_template` 事件类（net.h:22-33），字段为 `queue_mapping/skbaddr/vlan_tagged/.../len/name`——没有 `skb`。BTF/tracefs 的 format 文件列的就是这些字段，`args->skb` 在加载期就报 `unknown field`。查字段最快的办法：`cat /sys/kernel/tracing/events/net/netif_receive_skb/format`。
</details>

<details>
<summary>Q4：为什么不能用 kprobe 拼出"网卡到应用"的完整延迟时间线？</summary>

三个硬伤：①时钟域不同——kprobe 用的是本机 monotonic 时钟，而包的"到达时刻"权威定义在网卡硬件时钟（要 PTP 对齐）；②精度不够——kprobe 探针本身有开销和 skid，路径上的探针越多失真越大；③插桩改变被测系统（每个探针百 ns 级 + cache 污染）。端到端延迟的正解是 SO_TIMESTAMPING 硬件时间戳 + 应用侧收时间做差——只在两个端点各记一次，零中间插桩。
</details>

<details>
<summary>Q5：nstat 的 UdpInDatagrams 与 ethtool -S 的 rx_packets 对不上，中间可能发生了什么（按可能性排序）？</summary>

①GRO 合并：N 个包进协议栈算 1 次（最常见，per-packet vs per-skb 的口径差）；②XDP/过滤器丢弃：XDP_DROP 在协议栈之前，rx_packets 已计数；③softnet backlog 丢弃（softnet_stat 第 2 列会涨）；④校验错误（UdpInErrors 会涨）；⑤非 UDP 流量（多协议共用网卡，要先按协议过滤口径）。排查顺序：先看 softnet_stat 和 nstat 的错误计数，再看是否开了 GRO——多数情况是口径问题不是真丢。
</details>
