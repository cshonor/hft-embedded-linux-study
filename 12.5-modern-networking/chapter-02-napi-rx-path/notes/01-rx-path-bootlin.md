# 01 — 收包全路径：从网卡 DMA 到 socket 可读

> **Bootlin 课程模块：** RX Path
> **对应 Rosen:** Ch1（概述）/ Ch11（Layer 4 收包）
> **内核源码路径：** `net/core/dev.c`、`net/core/gro.c`、`net/ipv4/ip_input.c`、`net/ipv4/udp.c`

## 文档概述

Bootlin 的 RX Path 模块给出收包的完整链路。原笔记只有一张 9 步的图和一张延迟表，
本篇把它补到**内核函数级**：每一跳在哪发生、耗时怎么量、卡住了怎么定位。

本篇是本章的骨架，后续几篇都是这条链路某一段的纵深：

| 篇 | 覆盖的段 |
|----|---------|
| [02-napi](./02-napi.md) | 第 2–4 跳：NAPI 状态机与驱动 API |
| [03-napi-modern](./03-napi-modern.md) | 第 2–4 跳的现代变体：threaded NAPI、busy polling |
| [04-gro-gso](./04-gro-gso.md) | 第 7 跳：GRO 聚合与 GSO 分段 |
| [05-busy-poll-mechanism](./05-busy-poll-mechanism.md) | 第 2 跳与第 10 跳：干掉中断唤醒与调度延迟 |
| [06-queue-steering-rss](./06-queue-steering-rss.md) | 第 1–3 跳：包落到哪个队列、哪个核 |

---

## 完整链路（函数级）

```
【硬件】
 1. NIC 收帧，DMA 写入 Rx ring 描述符指向的内存（page_pool 分配的页）
        └─ 零 CPU 参与。所谓"零拷贝"说的从来不是这一段 —— DMA 本来就不经 CPU

【硬中断 / 上半部】
 2. NIC 拉中断线（MSI-X）
        └─ 驱动 ISR，例：ice_msix_clean_rings()  (drivers/net/ethernet/intel/ice/ice_main.c)
        └─ 只做两件事：关掉本队列的中断 + napi_schedule(&q_vector->napi)
        └─ 关键：这里**不收包**。把活推给下半部是 NAPI 的全部意义

【软中断 / 下半部】
 3. __do_softirq() → net_rx_action()            net/core/dev.c
        └─ 遍历 sd->poll_list，取出 napi_struct
        └─ napi_poll() → napi->poll()，即驱动注册的回调（例 ice_napi_poll）
        └─ 预算：单次 poll 最多 weight(64) 个包；整个软中断最多 netdev_budget(300) 个
        └─ 处理不完 → 不调用 napi_complete()，软中断重新调度自己继续收

【驱动 poll】
 4. 驱动从 Rx ring 取描述符，把 DMA 页包装成 skb 或 xdp_buff
        └─ ice_clean_rx_irq()  (ice_txrx.c)
        └─ 同时**回填新的描述符**（ replenish ），否则 ring 空了就 imissed

【XDP 挂载点 —— 最早的干预点】
 5. 若网卡已加载 XDP 程序，此时执行（在 skb 分配**之前**）
        ├─ XDP_DROP    → 页直接回收到 page_pool，不分配 skb（省 100-200ns）
        ├─ XDP_TX      → 改 MAC 后从同一口发回
        ├─ XDP_REDIRECT→ AF_XDP socket / CPUMAP / DEVMAP
        └─ XDP_PASS    → 继续往下
        → 详见 chapter-05-xdp-architecture

【sk_buff 分配】
 6. napi_build_skb() / napi_alloc_skb()   net/core/skbuff.c
        └─ 仅 XDP_PASS 的包才走。这是 XDP 能省下的最大一块

【GRO】
 7. napi_gro_receive()                    net/core/gro.c
        └─ 同流的小包合并成一个大 skb，减少协议栈遍历次数
        └─ **等合并窗口 = 等延迟**，HFT 必须关 → [04-gro-gso](./04-gro-gso.md)

【进入协议栈】
 8. netif_receive_skb() → __netif_receive_skb() → __netif_receive_skb_core()
        net/core/dev.c
        └─ 先跑 RPS（若开启）：enqueue_to_backlog() 转发到别的核
        └─ 再交付 tc ingress（sch_handle_ingress）  → chapter-09-tc-bpf
        └─ 最后按 ptype 分发：ETH_P_IP → ip_rcv()

【L3 交付】
 9. ip_rcv() → ip_rcv_core()              net/ipv4/ip_input.c
        └─ NF_HOOK(NFPROTO_IPV4, NF_INET_PRE_ROUTING)   ← nftables 挂载点
        └─ ip_rcv_finish() → ip_route_input_noref()（路由查找）
        └─ 本机 → ip_local_deliver() → ip_local_deliver_finish()
        └─ 组播 → ip_route_input_mc()，比单播多一次组播路由判定
                                              → [ch14/03 组播收包路径](../../chapter-14-tcp-udp-internals/notes/03-multicast-rx-path.md)

【L4 交付】
10. udp_rcv() → __udp4_lib_rcv()          net/ipv4/udp.c
        └─ 按四元组查 socket；组播要**给每个匹配的 socket 各复制一份 skb**
        └─ udp_queue_rcv_skb() → sock_queue_rcv_skb()
        └─ 挂到 sk->sk_receive_queue

【唤醒用户态】
11. sk->sk_data_ready() → sock_def_readable()
        └─ 唤醒阻塞在 recvmsg()/epoll 的进程 → 进程被调度上 CPU
        └─ 然后才是 copy_to_user 把数据拷出内核
```

---

## 延迟分解（含量测方法）

原笔记给了一张 7 行的表但没说怎么量。补上可复现的手段：

| 段 | 典型延迟 | 怎么量 |
|----|---------|--------|
| NIC 收帧 → 中断拉起 | 100–500 ns | `ethtool -C` 关合并后对比；逻辑分析仪测最准 |
| 硬中断 handler | 1–5 μs | `perf probe` 挂 ISR；或 `irq:irq_handler_entry/exit` tracepoint |
| 软中断排队等待 | 0–几百 μs ⚠️ | `net:napi_poll` tracepoint 的时间戳间隔 |
| 驱动 poll + skb 分配 | 100–300 ns | `perf record -e net:napi_poll` |
| XDP 程序 | 10–50 ns | 程序自己的 `bpf_ktime_get_ns()` 差值 |
| GRO | 100–500 ns（含等待窗口） | 开/关 GRO 对比 p99 |
| 协议栈（IP+UDP） | 500–2000 ns | `perf probe udp_rcv` 到 `sock_queue_rcv_skb` |
| 唤醒 + 调度 | 1–10 μs ⚠️ | `sched:sched_wakeup` → `sched:sched_switch` 差值 |
| copy_to_user | 取决于包大小 | `perf mem` 或 `copy_user` 相关 probe |

⚠️ 标出的两项是**抖动的主要来源**，不是代码问题而是调度问题 —— 诊断方法见
[../../chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md](../../chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md)

### 一条可以直接跑的观测命令

```bash
# 看每次 NAPI poll 处理了多少包、花了多久、有没有提前退出
bpftrace -e 'tracepoint:net:napi_poll { @[comm] = hist(args->work); }'

# 软中断的处理时长分布（>1ms 说明软中断被挤压或 budget 太小）
bpftrace -e 'tracepoint:irq:softirq_entry /args->vec==3/ { @s[tid]=nsecs; }
             tracepoint:irq:softirq_exit  /args->vec==3/ { if (@s[tid]) {
                 @us = hist((nsecs - @s[tid])/1000); delete(@s[tid]); } }'

# 驱动层丢包（Rx ring 满 / 描述符耗尽）
ethtool -S eth0 | grep -E 'missed|no_buf|drop|rx_out_of'
```

> `vec==3` 是 `NET_RX_SOFTIRQ`。软中断号在 `include/linux/interrupt.h` 里定义。

---

## 优化手段对照（补上"为什么"）

| 手段 | 省掉哪一段 | 典型收益 | 代价 |
|------|-----------|---------|------|
| 关中断合并 `ethtool -C eth0 rx-usecs 0` | 第 2 跳的等待 | 数百 ns | 中断数暴涨，CPU 上升 |
| threaded NAPI | 第 3 跳的软中断不确定性 | 降低 p99 | 多一次线程调度 |
| busy polling | 第 2 跳**和第 11 跳** | 3–8 μs ⭐ | 该核 100% |
| 关 GRO | 第 7 跳的合并窗口 | 0.5–1 μs | 吞吐下降（协议栈遍历次数变多） |
| XDP 早过滤 | 第 6–10 跳 | 0.5–2 μs | 要写 BPF，且失去协议栈 |
| AF_XDP 零拷贝 | 第 6–11 跳 | 5–10 μs ⭐ | 独占队列，网卡对该队列不再走内核栈 |
| DPDK 完全旁路 | 第 2–11 跳 | 再低一点 | **失去 TCP/路由/SSH 等一切内核功能** |

⭐ 两项是"不旁路就能拿到的最大收益"，也是
[05-busy-poll-mechanism](./05-busy-poll-mechanism.md) 的核心结论：
**先把它俩配到位再谈 DPDK**。

### 一个容易搞反的关系

关 GRO 降延迟，但**降的是每个包的延迟，不是整批的延迟**。
GRO 把 N 个小包合成 1 个大 skb 后，协议栈只遍历一次 —— 高 pps 下吞吐更好。
行情场景要的是"每个 tick 尽快到手"，所以关；
但如果你的瓶颈是"包太多处理不过来"（`softnet_stat` 的 squeeze 在涨），
关 GRO 反而会更糟。

---

## HFT 要点

- **收包延迟的大头不在协议栈里，而在"唤醒 + 调度"上**（第 11 跳）。
  优化协议栈代码收益有限，干掉中断和调度才是数量级的差别
- **XDP 的价值是"便宜地丢包"**，不是"便宜地收包"。
  99% 的流量你要丢，那在 skb 分配之前丢就是净赚
- **Rx ring 满 ≠ socket 队列满**，两者是不同的丢包点，定位命令也不同：
  - 驱动/Rx ring：`ethtool -S` 的 `missed` / `no_buf`
  - socket 队列：`netstat -su` 的 `receive buffer errors`、`ss -ulnp` 的 Recv-Q
  - 内核 UDP 层：`/proc/net/udp` 的 drops 列
- **组播没有重传**，丢一个包就是永久缺口，必须做序列号 gap 检测 →
  [ch14/03](../../chapter-14-tcp-udp-internals/notes/03-multicast-rx-path.md)

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| Rx 缓冲区 | 驱动各自管 | **page_pool 统一复用** → [chapter-04](../../chapter-04-page-pool/) |
| 干预时机 | 只能进协议栈后过滤 | **XDP 在 skb 分配之前** → [chapter-05](../../chapter-05-xdp-architecture/) |
| 下半部形态 | 只有软中断 | 软中断 / **threaded NAPI** / busy poll 三种 |
| 唤醒路径 | 中断唤醒 | busy polling 可完全绕过 → [05](./05-busy-poll-mechanism.md) |
| 收包批量 | 单 skb | `netif_receive_skb_list()` 批量分发 |

## 代码自测

<details>
<summary>Q1：为什么硬中断 handler 里不收包，非要推给 NAPI？</summary>

因为中断上下文关中断、不能睡眠、不能久留。
如果每个包都进一次中断，10G 网卡满速下每秒上千万个包，
CPU 会全部耗在中断上下文切换上（这叫 **receive livelock**）。
NAPI 的做法是：第一个包进中断 → **立刻关掉这个队列的中断** →
改成主动轮询把积压收完 → 收完再开中断。
于是高负载下自动退化成轮询，低负载下仍是中断驱动，两头都占。
</details>

<details>
<summary>Q2：`ethtool -S` 里 missed 和 no_buf 都在涨，分别说明什么？</summary>

- `missed`（如 `rx_missed`）：**PCIe/ring 侧** —— 网卡收到了但没地方写，
  通常是 CPU 收得不够快导致 Rx ring 描述符用完。加 ring size 或加速消费。
- `no_buf`：驱动拿不到新的缓冲区回填描述符。
  现代驱动用 page_pool，这一般意味着 page_pool 配置太小或内存吃紧。
两者的分界是"卡在硬件队列"还是"卡在缓冲区供给"。
</details>

<details>
<summary>Q3：`softnet_stat` 的第三列（squeeze/time_squeeze）一直涨，该怎么调？</summary>

`time_squeeze` 表示 NAPI 一次 poll 没干完就被软中断的时间预算（`netdev_budget_usecs`）
赶下去了。它涨说明软中断被挤压 —— 包进得比处理得快。
调这三处：`net.core.netdev_budget`（默认 300）、
`net.core.netdev_budget_usecs`（默认 2000μs）、
以及驱动侧的 `weight`（`ethtool -C` 部分驱动可调）。
但要注意：**这只是把积压往后推**，真正的解法是让消费端更快（→ busy poll / 旁路）。
</details>
