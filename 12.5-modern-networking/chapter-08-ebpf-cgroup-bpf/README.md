# Chapter 08: eBPF 与 cgroup BPF

> 来源：Bootlin（eBPF 网络）+ kernel-docs（BPF 类型系统 / verifier）+ LWN（XDP-BPF + cgroup BPF）
> 对标：Rosen（**无 eBPF**，3.x 只有 classic BPF，即 `struct sock_fprog` / `sk_filter`）
> 内核版本：以 **v6.6** 为准，机制、常量、行号均取自源码
> （`net/core/filter.c`、`net/core/dev.c`、`net/ipv4/ip_output.c`、`kernel/bpf/cgroup.c`、
> `kernel/bpf/verifier.c`、`kernel/bpf/syscall.c`、`include/uapi/linux/bpf.h`）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [ebpf-net-bootlin](notes/01-ebpf-net-bootlin.md) | **全景 + 实操**：五个网络 hook 点的源码级位置、程序类型 × 挂载点 × 工具横表、三条工具链（libbpf+CO-RE / xdp-tools+tc / bpftool）、四层观测体系、`run_time_ns`/`run_cnt` 测量法、10 条故障对照表 |
| 2 | [bpf](notes/02-bpf.md) | **类型系统 + verifier**：33 种程序类型全表、33 种 map 类型分类、`CAP_BPF` 权限模型、verifier 三条硬规则、**3 处文档与代码不一致的纠正**、helper 能力集对比（XDP 23 vs tc 81） |
| 3 | [xdp-bpf](notes/03-xdp-bpf.md) | **XDP 程序**：`struct xdp_md` 的 6 个字段、五个返回码（`XDP_ABORTED == 0` 陷阱）、`adjust_head` 的 3 条硬约束 + 包指针失效规则、**native/generic 的真实代价（generic 会 pskb_expand_head + skb_linearize，且可能静默丢包）**、成本结构分析 |
| 4 | [cgroup-bpf](notes/04-cgroup-bpf.md) | **cgroup 系列**：attach 类型全表、`BPF_F_ALLOW_OVERRIDE`/`MULTI`/`REPLACE` 的继承与叠加规则（含 UAPI 原文 Ex1 逐行解释）、`SETSOCKOPT` 返回码的四条语义、16 个 `sock_ops` 事件 + 回调订阅开关、7 条故障对照表 |

## 本篇的六个核心结论

1. **⭐ cgroup ingress 不在「包进协议栈时」，而在 socket 入队时。**
   `BPF_CGROUP_RUN_PROG_INET_INGRESS` 在 v6.6 只有一处调用点：
   `sk_filter_trim_cap()`（`net/core/filter.c:138`），位置是
   `sock_queue_rcv_skb()` → `sk_filter()`。走完路由、Netfilter、UDP/TCP 栈、
   socket 查找之后才轮到它。**被转发的包、未加入的组播、没有 socket 的包根本不会经过。**
   → 想省 CPU 的过滤必须放在 XDP 或 tc，放 cgroup 等于什么都没省。

2. **⭐ cgroup egress 在 Netfilter POST_ROUTING 之后。**
   `ip_finish_output()`（`net/ipv4/ip_output.c:314/318`）是
   `NF_HOOK_COND(..., NF_INET_POST_ROUTING, ..., ip_finish_output)` 的 **okfn**。
   所以 SNAT/MASQUERADE 先跑，cgroup egress 看到的是 SNAT 之后的地址。
   组播走**独立路径** `ip_mc_finish_output()`（`ip_output.c:330/337`）——单播和组播要各测一遍。

3. **⭐ `XDP_ABORTED == 0`，这是 XDP 最危险的默认值。**
   任何「忘了 return」或「把 helper 的负 errno 当返回码」的程序，返回值默认 0 = `XDP_ABORTED` = 丢包。
   加载不报错、编译器不警告、tcpdump 看不到（AF_PACKET 在 XDP 之后）、
   `ethtool -S` 也看不到（驱动统计在 XDP 之后才更新）。
   **唯一的可见处是 `tracepoint:xdp:xdp_exception`。**

4. **⭐ generic XDP 会为每个包补做 `pskb_expand_head()` + `skb_linearize()`。**
   `netif_receive_generic_xdp()`（`net/core/dev.c:4941`）为了「模拟」native 的
   线性 + `XDP_PACKET_HEADROOM`(256) 保证，对不满足条件的包做内存分配 + 数据拷贝，
   **分配失败则 `goto do_drop` 静默丢包**。
   → generic 只用于验证逻辑正确性，**不能用于性能评估或生产**；用 `xdpdrv` 而不是 `xdp` 加载。

5. **⭐ cgroup BPF 不做短路：所有程序都会执行。**
   UAPI 注释原文（`include/uapi/linux/bpf.h:1117`）：
   *"All eligible programs are executed regardless of return code from earlier programs."*
   这与 Netfilter（`NF_DROP` 短路）和 tc（`TC_ACT_SHOT` 短路）**完全相反**。
   → 想表达「白名单优先于黑名单」必须靠共享 map 或合并成一个程序。

6. **⭐ verifier 文档注释已过时，实际限流是 100 万而非 64k。**
   `kernel/bpf/verifier.c:50` 的注释说「第二遍 limited to 64k insn」「branches limited to 1k」，
   但 v6.6 代码里这两个数字**都不存在**：实际是
   `++env->insn_processed > BPF_COMPLEXITY_LIMIT_INSNS`（`verifier.c:16455`），
   而 `BPF_COMPLEXITY_LIMIT_INSNS = 1000000`（`include/linux/bpf.h:1723`）。
   另外 `BPF_MAXINSNS = 4096` 定义在 **`include/uapi/linux/bpf_common.h:54`**
   （不在 `bpf.h`，所以很多人 grep 不到），且**只对无特权调用者生效**。

## HFT 关联

- **选 hook 就是选「能为这个包付多少 CPU」**：
  XDP（驱动层，无 skb）< tc ingress（有 skb，L3 前）< cgroup ingress（走完整个协议栈 + socket 查找）
- **cgroup BPF 的真实价值是「零侵入可观测」**，不是性能：
  `sock_ops` 的 `srtt_us` / `rtt_min` / `snd_cwnd` / `RETRANS_CB` / `STATE_CB`
  让你在**不碰交易程序一行代码**的前提下采到 TCP 健康度，
  比 `ss -i` 轮询（有采样间隔、会漏瞬时抖动）可靠得多
- **`sock_ops` 的高频回调必须先订阅**：`RTT_CB` / `RTO_CB` / `RETRANS_CB` / `STATE_CB`
  默认不触发，要在 `*_ESTABLISHED_CB` 里调 `bpf_sock_ops_cb_flags_set()`
- **`bpf_redirect_map()` 的 flags 永远显式写 `XDP_PASS`**，不要写 0
  （flags 低位 = 未命中时的返回码，写 0 就是 `XDP_ABORTED` = 静默丢弃 ARP/ICMP/SSH）
- **`run_time_ns`/`run_cnt` 是成本归因的第一站**，但它自带
  `sched_clock()` ×2 + 关中断 seqcount 的埋点开销，可能比程序本身还贵，**测完要关**
- **v6.6 起 tc 有两条路**：tcx（bpf_link 语义，推荐新项目）与 legacy clsact；
  `sch_handle_ingress()` 里 **tcx 先跑、legacy 后跑**（`net/core/dev.c:4023-4030`）
- **进程隔离用 cgroup，别用 iptables `-m owner`**：后者只能按 uid/gid 固定属性匹配，
  前者可以跑任意 BPF 逻辑 + 查 map 状态
- **⚠️ socket 的 cgroup 归属终身不变**：创建时继承，之后进程迁移也不变。
  做灰度切换要么重建连接，要么新旧 cgroup 都挂一份

## 交叉引用

- `12.5-modern-networking/chapter-01-net-stack-architecture/notes/01-net-stack-architecture.md`：
  hook 点总顺序（本篇的所有位置都是在它的骨架上插桩）
- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP 基础架构，本篇第 3 节的能力边界由此而来
- `12.5-modern-networking/chapter-06-af-xdp/`：XSKMAP 目的地，`XDP_REDIRECT` 到 AF_XDP 的完整机制
- `12.5-modern-networking/chapter-07-xdp-redirect-dpdk/notes/01-xdp-redirect.md`：
  `xdp_do_flush()` 的三步批量语义
- `12.5-modern-networking/chapter-09-tc-bpf/`：tc-BPF（81 个 helper，XDP 只有 23 个）
- `12.5-modern-networking/chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md`：
  延迟测量方法（本篇刻意不给 cycles 数字的原因）
- `06.7-bpf-observability/`：eBPF 可观测性体系
