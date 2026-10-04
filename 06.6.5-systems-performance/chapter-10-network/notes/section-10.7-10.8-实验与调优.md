## 10.7–10.8 实验与调优

> 章节导航：[10.6 观测工具](./section-10.6-观测工具.md) · 上一篇 ← · [本章导读](../README.md)

**本节讲什么**：网络微基准与故障注入（iperf3/netperf/netem）、sysctl 调优的语义与「低延迟 vs 高吞吐参数集相反」的原因、套接字选项的机制与 HFT 选择。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | iperf3 测的是**栈上限**，不是业务延迟 | 应用级测试另做 |
| 2 | netem 是**混沌工程**的廉价入口 | delay/loss/reorder 注入 |
| 3 | **低延迟与高吞吐的 sysctl 相反** | Netflix 表不能抄到 HFT |
| 4 | TCP_NODELAY 是 HFT 的**默认** | Nagle 与延迟互斥 |
| 5 | busy poll 用 CPU 换延迟 | 与 DPDK poll 同哲学 |

---

### 一、微基准与故障模拟

| 工具 | 用途 | 注意 |
|------|------|------|
| **`iperf3`** | TCP/UDP 最大吞吐 | 单流 vs `-P` 多流测不同问题（单流受窗口/单核限制，多流测聚合） |
| **`netperf`** | RPC 风格 RTT（request-response） | 比 iperf 更像交易型负载 |
| **`tc netem`** | 注入 **delay/loss/reorder** | 混沌测试——验证策略对烂网络的鲁棒性 |

```bash
iperf3 -c server -t 30 -P 4        # 4 流并行测聚合吞吐
# 注入 2ms 延迟 + 0.1% 丢包 + 少量乱序（测试环境！）
tc qdisc add dev eth0 root netem delay 2ms loss 0.1% reorder 0.5%
tc qdisc del dev eth0 root         # 撤销
```

**HFT**：共置 baseline 用 **iperf + 应用级 ping 订单通道**双口径（iperf 是栈上限，应用 ping 才含编解码/策略）；netem 在测试环境验证**策略鲁棒性**（丢包时降级逻辑、重连超时）——生产绝不上 netem。

### 二、系统级 sysctl：语义与方向

**为什么低延迟与高吞吐参数集相反**——大 buffer 吞吐友好（吸收抖动、攒大包），但**排队延迟上升**（bufferbloat）；小 buffer 延迟友好（早丢早重传），吞吐受损。Netflix（高吞吐 Web）与 HFT（低延迟交易）在这根曲线上取相反的点。

| 参数 | 高吞吐方向 | 低延迟方向 | 说明 |
|------|-----------|-----------|------|
| `net.core.netdev_max_backlog` | ↑ | 适度（过大=排队） | 入口软中断队列 |
| `net.core.somaxconn` | ↑ | ↑（够用即可） | accept 队列上限 |
| `net.ipv4.tcp_max_syn_backlog` | ↑ | ↑（够用） | SYN 队列 |
| `tcp_rmem` / `tcp_wmem` | ↑↑ | **保守** | TCP 窗口——大=吞吐小=延迟 |
| `rmem_max` / `wmem_max` | ↑ | 按需 | socket buffer 上限 |
| `tcp_congestion_control` | `bbr` | **cubic/无所谓**（内网无拥塞） | BBR 对有损公网好；共置内网丢包≈故障不是拥塞 |
| `tcp_tw_reuse` | 1 | 理解再开 | TIME_WAIT 端口重用（旧数据残留风险） |
| `tcp_notsent_lowat` | 默认 | **调小**（如 16K） | 限制 notsend 排队——低延迟 TCP 的关键参数 |
| `tcp_low_latency`（已移除） | — | 历史 | 老内核提示，现代内核无此旋钮 |

**`tcp_notsent_lowat` 是低延迟 TCP 的隐藏王牌**：限制内核里「应用已写但未发送」的字节数——超出即让 write() 返回/EPOLLOUT 暂时不就绪，把排队从内核搬回应用可控层。发单通道设小值 = 延迟上限可控。

**纪律**：与 [14-HFT ch05/ch06](../../../14-hft-engineering/) 合并成**单一 sysctl runbook**——每条注明动机、预期效果、回滚方法；变更走对照实验（改一条测一条，[ch12 方法论](../../chapter-12-benchmarking/)）。

### 三、套接字选项（应用层）

| 选项 | 机制 | HFT |
|------|------|-----|
| **`TCP_NODELAY`** | 禁 Nagle——小包立即发 | **发单/低延迟默认开** |
| `TCP_CORK` | 聚合小包攒大段 | 批量非紧急数据（与 NODELAY 相反） |
| **`SO_BUSY_POLL`** / `epoll` busy poll | 读路径轮询代替中断等待 | 降 latency、增 CPU 占用 |
| **`SO_REUSEPORT`** | 多进程 bind 同端口，内核分流 | 收包多进程扩展 |
| `SO_PRIORITY` | 设置 skb 优先级（配合 qdisc） | 流量分级 |
| 非阻塞 + epoll | 事件驱动 | [ch5](../../chapter-05-applications/) · [UNP](../../../03.5-unix-network-api/) |

**Nagle 算法 vs TCP_NODELAY 的机制**：Nagle 说「有未 ACK 的小包在外时，新小包攒着」——防小包洪泛（1988 年的善意）对交互延迟是灾难（一个未 ACK 段就能把后续发单拖住一个 RTT）。HFT 一切小包通道默认 `TCP_NODELAY`；聚合需求由应用显式控制（自己攒批，不交给内核猜）。

**busy poll 的哲学**：中断/睡眠唤醒有 µs 级固定成本；`SO_BUSY_POLL`（或 epoll 的 busy poll 模式）让 CPU 原地轮询 NIC——**用 100% CPU 换几 µs 延迟**。与 DPDK 的 poll mode（[13-DPDK](../../../13-dpdk/)）、io_uring 的 polled 模式（[ch9 io_poll](../../chapter-09-disks/notes/section-9.4-硬件与软件架构.md)）同一家族：低延迟工程的终极旋钮都是「中断换轮询」。

### 四、调优优先级（与磁盘同构）

| 优先级 | 手段 | 成本 |
|--------|------|------|
| 1 | 应用层：NODELAY、异步、批量自主、连接复用 | 代码 |
| 2 | 拓扑：中断亲和、RPS/RFS、队列分流（[ch6](../../chapter-06-cpus/)） | 配置 |
| 3 | sysctl：按低延迟口径 | 配置 |
| 4 | 硬件：更快的 NIC/交换机、直连拓扑 | 预算 |
| 5 | 旁路：DPDK/XDP（把内核栈整个搬走） | 架构改造 |

### HFT / 嵌入式关联

- **共置参数集 vs 云参数集分开维护**：同一份 sysctl 基线在两种环境下都次优——runbook 里明确「这台机器跑哪套」。
- **应用级 ping 是验收标准**：订单通道 round-trip（发单→回执）的 P99——iperf 只是容量参考（[ch12 拷问](../../chapter-12-benchmarking/notes/section-12.4-基准测试拷问Benchmark-Questions.md)：测量点在哪）。
- **netem 验收鲁棒性**：丢包 0.1% 时策略的降级行为、重连风暴——上线前必测。
- **嵌入式**：无 tc netem 权限时，用 iptables 的随机 drop 或 EMEDIUM 模拟——目的相同（注入劣化验证鲁棒性）。

### 衔接

- 上一节：[10.6 观测工具](./section-10.6-观测工具.md)
- 关联：[ch6 中断与亲和](../../chapter-06-cpus/)、[ch12 基准测试](../../chapter-12-benchmarking/)、[14-HFT ch06 低延迟网络协议](../../../14-hft-engineering/chapter-06-low-latency-network-protocol/README.md)、[13-DPDK](../../../13-dpdk/)（旁路终点）、[02-CSAPP ch11](../../../02-computer-systems/chapter-11-network-programming/)

---

### 常见陷阱

1. **盲抄 Netflix sysctl**——Netflix 面向高吞吐 Web（大 buffer/BBR），HFT 面向低延迟（小 buffer/NODELAY/notsent_lowat），参数集相反。
2. **tcp_tw_reuse 不理解就开**——TIME_WAIT 端口重用有旧数据残留风险。
3. **iperf3 当业务 benchmark**——测 TCP 栈吞吐上限，不含编解码/策略/发单；验收用应用级 ping。
4. **netem 上生产**——故障注入只属于测试环境；忘了删 netem 规则是经典事故。
5. **Nagle 没关就抱怨延迟抖**——一个未 ACK 段拖住后续小包一个 RTT；TCP_NODELAY 是第一检查项。

<details>
<summary>自测题（点击展开）</summary>

1. 为什么不能直接抄 Netflix 的 sysctl？
   <details><summary>答</summary>Netflix 高吞吐 Web（大 buffer 吸收抖动、BBR 抗公网丢包）；HFT 低延迟（小 buffer 控排队、内网丢包=故障不是拥塞）——同一根吞吐-延迟曲线取相反的点。</details>
2. tcp_notsent_lowat 为什么是低延迟 TCP 关键参数？
   <details><summary>答</summary>限制「应用已写但未发送」的内核排队字节——把排队从内核搬回应用可控层，延迟上限可控。</details>
3. Nagle 算法和 TCP_NODELAY 的机制？
   <details><summary>答</summary>Nagle：有未 ACK 小包在外时新小包攒着（防洪泛）——交互延迟灾难；NODELAY 禁掉它，小包立即发，聚合由应用自主。</details>
4. busy poll 为什么降延迟？
   <details><summary>答</summary>中断/睡眠唤醒有 µs 级固定成本；原地轮询 NIC 用 100% CPU 换掉这段——与 DPDK/io_uring polled 同一哲学：中断换轮询。</details>
5. netem 的正确使用姿势？
   <details><summary>答</summary>仅测试环境：注入 delay/loss/reorder 验证策略鲁棒性（降级逻辑/重连超时）；生产禁用；用后即删。</details>

</details>


---

← [本章导读](../README.md)
