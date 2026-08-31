# Chapter 09: TC 与 BPF

> 来源：Bootlin（TC 概述）+ LWN（tc-BPF 系列）+ **v6.6 源码逐条核对**
> 对标：Rosen Ch6（Traffic Control）/ Ch9（Netfilter）——3.x 的 TC → 6.x 的 tc-BPF
> 内核版本：以 **v6.6** 为准，机制、常量、行号均取自源码
> （`net/core/dev.c`、`net/core/filter.c`、`net/sched/sch_ingress.c`、
> `include/net/sch_generic.h`、`include/uapi/linux/pkt_cls.h`、`include/uapi/linux/bpf.h`）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [tc-bootlin](notes/01-tc-bootlin.md) | **qdisc 体系 + HFT 出向调度**：纠正「filter 在 qdisc 之后」的经典错误图、`__dev_queue_xmit()` 的真实出向路径、qdisc 三角色、fq/etf/prio/tbf 四种 qdisc 的选型对比、两套 HFT 出向配置模板、etf 静默丢包排查清单 |
| 2 | [tc-bpf](notes/02-tc-bpf.md) | **tc-BPF 程序**：v6.6 双机制（tcx vs legacy clsact，串联执行顺序）、legacy 返回码全表与 `da` 模式、`__sk_buff` 可写性规则（对照 `SOCKET_FILTER`）、81 个 helper vs XDP 的 23 个、行情流打标记 + 延迟测量完整示例 |

## 本篇的核心结论

1. **⭐ tc egress 在 qdisc 之前**——这是本篇最重要的位置认知。
   出向路径是 **Netfilter egress → tc egress → qdisc → 驱动**（`dev.c:4306` vs `:4311`），
   所以 tc-BPF 能做的是**分类 + 丢包**，**整形必须靠 qdisc**。
   BPF 和 qdisc 是 tc 的两半：BPF 决定「这个包属于哪类、要不要丢」，qdisc 决定「什么时候发」。

2. **⭐ filter 不是流水线阶段，是「挂」在 qdisc 上的。**
   「socket → qdisc → class → filter → driver」是流传很广的错误画法。
   正确理解：filter 挂在 qdisc/class 上做分类决策，clsact 是不带队列的纯挂载点。

3. **⭐ v6.6 起 tc-BPF 有两条挂载路径，且会串联执行。**
   `sch_handle_ingress()`（`dev.c:4005`）里 **tcx 先跑、legacy clsact 后跑**；
   tcx 的 `TCX_NEXT` 继续链，其他返回值终止链。新项目用 tcx
   （bpf_link 语义、可原子替换、无需 clsact qdisc），存量继续 legacy。

4. **⭐ tcx 的「忘 return」= 放行（0 = `TCX_PASS`），与 XDP 完全相反**（0 = `XDP_ABORTED` = 丢包）。
   两边安全默认值不同，跨程序类型搬代码时要特别注意。
   tcx 只认 `-1/0/2/7` 四个返回值，其他值被归一化成 `TCX_NEXT`。

5. **⭐ `etf` 是唯一能给「确定性发送时刻」的 qdisc，但硬约束多。**
   必须 `CLOCK_TAI`、socket 必须 `SO_TXTIME`、硬件卸载需要 TSN 网卡，
   三条任一不匹配就是**静默丢包**（`skip_sock_check` 除外）。

6. **⭐ tcpdump 能看到被 tc ingress 丢弃的包，看不到被 XDP 丢弃的。**
   这是调试时区分「XDP 丢的还是 tc 丢的」的第一判据。
   丢包计数走 `SKB_DROP_REASON_TC_INGRESS/EGRESS` + `tracepoint:skb:kfree_skb`。

## HFT 关联

- **选型基线**：tc ingress（有 skb，L3 前，能改写）补足 XDP 做不到的**改包/重定向/多队列导向**；
  XDP 仍是无 skb 的最快路径，两者不是替代关系
- **`clsact` 不引入排队延迟**：挂 tc-BPF 用 clsact（无队列），整形另配 fq/tbf/prio
- **`fq` 的 pacing 是降低出向 burst 的主要手段**：burst 打爆交换机 buffer 造成的
  排队抖动（几十 µs~ms 级）比 CPU 优化重要得多
- **`prio` 会饥饿低优先级**：永远给高优先级流量配上限速（下层 tbf / fq）
- **多队列网卡上 `skb->queue_mapping` 可在 tc egress 设置**，影响 TX 队列选择
  （发生在 `netdev_core_pick_tx` 之前，但受 `netdev_xmit_txqueue_skipped()` 约束）
- **`local_port` 主机字节序、`remote_port` 网络字节序**——做 map key 时必须分别处理
- **`SO_ATTACH_BPF` 的 socket filter 连包内容都读不到**（禁访 `data`/`data_end`），
  内容过滤必须用 tc-BPF 或 XDP

## 交叉引用

- `12.5-modern-networking/chapter-03-tx-path-skbbuff/`：TC 在发包路径的位置
- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP（tcpdump 看不到的那个 hook）
- `12.5-modern-networking/chapter-08-ebpf-cgroup-bpf/`：eBPF 通用框架与 verifier
- `12.5-modern-networking/chapter-15-debugging-perf-tuning/`：延迟测量
