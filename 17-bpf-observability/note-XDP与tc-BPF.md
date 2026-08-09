# XDP 与 tc-BPF · HFT 延伸

> **BPF Performance Tools** · Brendan Gregg · **精读**

## XDP 是什么

XDP（eXpress Data Path）是 Linux 内核提供的**高速包处理路径**——在网卡驱动收到包后、进入内核网络栈之前，运行 BPF 程序做早期处理（丢弃、转发、重定向、允许通过）。

| 特性 | 说明 |
|------|------|
| **位置** | 网卡驱动 RX 路径，SKB 分配之前 |
| **性能** | 每包开销极低（~10ns 级），适合高 PPS |
| **模式** | native（网卡驱动原生支持，最快）/ generic（任何网卡，较慢）/ offloaded（网卡 ASIC，最快） |
| **动作** | `XDP_PASS`（放行）/ `XDP_DROP`（丢弃）/ `XDP_TX`（回发）/ `XDP_REDIRECT`（重定向）/ `XDP_ABORTED`（异常） |

## XDP vs 内核网络栈 vs DPDK

| 维度 | 内核网络栈 | XDP | DPDK |
|------|-----------|-----|------|
| **路径** | 完整 TCP/IP 栈 | 驱动层早期处理 | 用户态 PMD |
| **延迟** | 微秒级 | 亚微秒级 | 纳秒级 |
| **灵活性** | 完整协议栈 | 包过滤/转发/丢弃 | 完全可编程 |
| **CPU 占用** | 内核线程 | 内核上下文 | 专用核 100% 轮询 |
| **观测手段** | BCC/bpftrace | bpftool + xdpdump | rte_eth_stats |
| **适用场景** | 通用网络 | DDoS 防护、负载均衡 | HFT 极低延迟 |

## tc-BPF 是什么

tc-BPF 是在 Linux 流量控制（tc）子系统中挂载 BPF 程序——在 qdisc（队列规则）层做包分类、修改、重定向。

| 特性 | 说明 |
|------|------|
| **位置** | qdisc 层（XDP 之后，协议栈之前） |
| **能力** | 包修改（mangle）、分类（classify）、重定向（redirect） |
| **优势** | 比 XDP 更灵活（可修改包内容），比协议栈更早（低延迟） |
| **典型用途** | 负载均衡（tc-redirect）、流量整形、包标记 |

## HFT 场景应用

### 1. XDP 早丢弃（减少无关包中断）

```c
// 丢弃非交易端口流量，减少 HFT 核上的网络中断
SEC("xdp")
int xdp_filter(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    struct ethhdr *eth = data;
    if ((void*)(eth+1) > data_end) return XDP_PASS;
    if (eth->h_proto != htons(ETH_P_IP)) return XDP_PASS;
    struct iphdr *ip = (void*)(eth+1);
    if ((void*)(ip+1) > data_end) return XDP_PASS;
    if (ip->protocol != IPPROTO_UDP) return XDP_PASS;
    struct udphdr *udp = (void*)(ip+1);
    if ((void*)(udp+1) > data_end) return XDP_PASS;
    // 只放行交易端口
    if (ntohs(udp->dest) == 9090) return XDP_PASS;
    return XDP_DROP;
}
```

### 2. tc-BPF 收发路径优化

```bash
# 将 HFT 应用的流量重定向到专用队列
tc qdisc add dev eth0 clsact
tc filter add dev eth0 ingress bpf da obj tc_redirect.o sec ingress
```

### 3. XDP 统计观测

```bash
# 查看已加载的 XDP 程序
bpftool prog show | grep xdp

# XDP 丢包/放行计数
bpftool prog show id <ID> -j | python3 -m json.tool

# 使用 xdpdump 抓包（不影响 XDP 处理）
xdpdump -i eth0 --rx-capture entry,exit
```

### 常见陷阱

1. **混淆 XDP native 和 generic 模式** — native 模式需要网卡驱动支持（如 mlx5、i40e），generic 模式在任何网卡可用但性能差 3-5 倍；HFT 生产环境必须用 native 或 offloaded
2. **以为 XDP 能做完整 TCP 处理** — XDP 在协议栈之前运行，只能做包级过滤/转发/丢弃；需要 TCP 状态机（重传、流控）必须走内核栈或 DPDK 用户态栈
3. **在 XDP 程序中做复杂逻辑** — XDP 程序在驱动 RX 路径上同步执行，复杂逻辑会阻塞后续包处理导致丢包；XDP 程序应尽量简单（查表、比较、快速决策）

<details>
<summary>📝 自测题（点击展开）</summary>

1. **XDP 和 tc-BPF 在网络包处理路径上的位置有什么区别？**

   <details>
   <summary>参考答案</summary>

   XDP 在网卡驱动 RX 路径上、SKB（socket buffer）分配之前运行——最早的包处理点，延迟最低。tc-BPF 在 qdisc（队列规则）层运行——SKB 已分配，在协议栈之前但比 XDP 晚。区别：XDP 更早更快但能力有限（不能修改包内容、不能查 TCP 状态）；tc-BPF 更灵活（可修改包、可分类）但延迟略高。HFT 可组合使用：XDP 做粗过滤，tc-BPF 做精细分类。
   </details>

2. **为什么 HFT 生产环境必须用 XDP native 模式而非 generic？**

   <details>
   <summary>参考答案</summary>

   Native 模式在网卡驱动 RX 路径中直接调用 XDP 程序，包数据在 DMA 缓冲区中原地处理，零拷贝。Generic 模式在 SKB 分配后调用 XDP 程序，额外有 SKB 分配和释放开销。性能差异：native ~10ns/packet，generic ~50ns/packet（3-5 倍差距）。HFT 微秒级延迟预算中，50ns/packet 的额外开销在高 PPS 下会累积。需确认网卡驱动支持 native XDP（如 mlx5、i40e、ixgbe）。
   </details>

3. **XDP 程序中为什么不能做复杂逻辑（如查数据库、做字符串匹配）？**

   <details>
   <summary>参考答案</summary>

   XDP 程序在驱动 RX 路径上同步执行——处理完一个包才能处理下一个。如果 XDP 程序耗时 100 微秒，后续所有包都要等待，导致 ring buffer 溢出丢包。XDP 程序受 verifier 限制（无阻塞 I/O、无系统调用、有指令上限），只能做：查 BPF Map（O(1) 查找）、比较字段、快速决策。复杂逻辑应放到 tc-BPF 或用户态处理。原则：XDP 程序执行时间应在微秒以内。
   </details>

</details>

## 相关

- [chapter-10-网络.md](./chapter-10-networking/)
- [15-Advanced note-XDP](../15-dpdk/02-Advanced-Book/notes/note-XDP与DPDK对照.md)
- [15-DPDK](../15-dpdk/)
