# 02 — XDP vs DPDK：旁路路线的真正分野

> **对应 Rosen:** 无（XDP 4.8+、AF_XDP 4.18+、DPDK 独立项目）
> **内核版本：** 内核侧以 **v6.6** 为准
> **本篇立场：** 不比"谁更快"，只讲**结构性差异**。
> 任何具体数字都依赖硬件/驱动/内核版本/包长，**必须自己测**（第十节给方法）。

---

## 文档概述

[01 篇](01-xdp-redirect.md)讲了 XDP_REDIRECT 的机制，
本篇回答选型问题：什么时候用 XDP（+AF_XDP），什么时候用 DPDK。

网上绝大多数 "XDP vs DPDK" 对比表都在比延迟数字，而那些数字
**几乎无法复现**——它们来自特定的网卡、特定的驱动版本、特定的包长和特定的
测试方法。本篇换一个轴：

> **分野是"谁拥有网卡"，不是"谁更快"。**

| 笔记 | 侧重 |
|------|------|
| [01 XDP_REDIRECT](01-xdp-redirect.md) | redirect 的三步语义与四个目的地 |
| **02（本篇）** | 旁路路线选型、所有权模型、混合架构、测量方法 |

---

## 一、先纠正三个流传很广的错误说法

| 常见说法 | 事实 |
|---------|------|
| "AF_XDP 零拷贝是 page_pool 映射" | ❌ **零拷贝不用 page_pool**。UMEM 由内核 `xp_dma_map()` 独立映射，pool 有自己的 `dma_pages` 数组（`include/net/xsk_buff_pool.h:68`）。"AF_XDP 基于 page_pool"只在 **copy 模式**下成立（那条路径上驱动确实用 page_pool）。详见 [chapter-04/01](../../chapter-04-page-pool/notes/01-page-pool.md) |
| "XDP 不需要 hugepage" | ⚠️ 内核不强求，但**UMEM 强烈建议放 hugepage**。XDP 驱动侧用 page_pool（4K 页），AF_XDP 的 UMEM 是用户态内存——你放 4K 页上也能跑，只是 TLB miss 更多 |
| "DPDK 不需要内核" | ⚠️ DPDK **需要 UIO/VFIO 内核模块**做 IOMMU 映射和中断屏蔽，只是**数据路径**不进内核。没有内核它连网卡都够不着 |

---

## 二、核心分野：谁拥有网卡

```
【XDP / AF_XDP】内核拥有网卡，你只是"插"了一段程序
┌──────────────────────────────────────────────────┐
│  应用（你的策略）                                  │
│       ↑ AF_XDP（UMEM，仅指定队列）                 │
│  ─────┼────────────────────────────────────────   │
│  内核：驱动 / NAPI / XDP / 协议栈 / 路由 / nftables │
│       ↑                                          │
│  网卡（内核驱动管理，队列 0 给 AF_XDP，其余照常）    │
└──────────────────────────────────────────────────┘

【DPDK】用户态拥有网卡，内核被摘掉
┌──────────────────────────────────────────────────┐
│  应用（含 PMD，直接操作硬件描述符环）               │
│       ↑ rte_mbuf + hugepage mempool                │
│  ─────┼────────────────────────────────────────   │
│  VFIO/UIO（只做 IOMMU 映射和权限）                  │
│       ↑                                          │
│  网卡（整张被接管，内核看不到它）                    │
└──────────────────────────────────────────────────┘
```

| 问题 | XDP / AF_XDP | DPDK |
|------|-------------|------|
| **网卡归谁管** | **内核驱动** | **用户态 PMD** |
| 内核还能看到这张卡吗 | ✅ 能（`ip link`、`ethtool`、路由表都正常） | ❌ 不能（`ip link` 里它还在，但收发全走 PMD） |
| 旁路粒度 | **按队列**（队列 0 旁路，1~N 照常走栈） | 整张卡 |
| 中断/NAPI | 内核的 NAPI 负责收包，你挂程序 | 你自己 100% 轮询，没有 NAPI |
| 描述符环归谁写 | 驱动 | **你的代码** |
| Hugepage | 可选（建议） | **必需** |
| 驱动兼容 | 用内核主线驱动，**网卡型号跟着内核走** | 需要 DPDK 里有对应 PMD，**版本要对齐** |

**这条轴线决定了一切其他差异。**

---

## 三、逐维度对比

| 维度 | XDP + AF_XDP | DPDK |
|------|-------------|------|
| **运行位置** | 内核态（驱动 NAPI 上下文） | 用户态（轮询线程） |
| **网卡控制** | 内核驱动 | 用户态 PMD（VFIO/UIO） |
| **旁路粒度** | **按队列** | 整张卡 |
| **sk_buff** | 不分配（XDP 在 skb 之前） | 不存在（rte_mbuf） |
| **协议栈** | 可共存（其他队列/其他协议照常） | 不存在（想用 TCP 得自己实现或用 F-Stack/mTCP） |
| **内存** | UMEM（用户态，建议 hugepage）+ page_pool（驱动侧） | hugepage mempool（**GB 级预留**） |
| **CPU 模型** | NAPI 驱动（可 busy poll 消除中断） | **100% 轮询**，一个核被吃掉 |
| **延迟构成** | DMA → NAPI 调度 → XDP → 用户态 | DMA → PMD 轮询 → 用户态（**少一次 NAPI 调度**） |
| **批处理** | 内核决定（`XDP_BULK_QUEUE_SIZE`=16、NAPI weight 默认 64） | **你自己决定**（`rte_eth_rx_burst()` 的 nb_pkts） |
| **部署** | 加载 BPF 程序 + bind socket | 绑 VFIO + 配 hugepage + NUMA 对齐 + 隔离核 |
| **调试工具** | `bpftool`、`xdp-tools`、`ethtool`、`perf`、`dropwatch` | `dpdk-proc-info`、`testpmd`、`dpdk-telemetry` |
| **升级耦合** | 跟内核版本（BTF/CO-RE 缓解） | 跟 DPDK 版本 + 内核（VFIO 接口） |
| **故障爆炸半径** | 程序 bug → 该队列丢包，其他队列正常 | PMD bug → **整张卡不通** |
| **运维可见性** | 网卡在内核里，`ip`/`ethtool`/`ss` 全可用 | 内核看不到流量，**所有常规工具失效** |

---

## 四、为什么 DPDK 在极限延迟上有结构性优势

不是"DPDK 代码写得更好"，而是**它少了几跳**：

```
【AF_XDP 零拷贝】
  NIC DMA → UMEM
      → 网卡发中断（或 busy poll 主动驱动）
      → NAPI poll 被调度                    ← ⚠️ ① 一次调度
      → NAPI 循环跑 XDP + redirect
      → NAPI poll 结束 → xdp_do_flush()      ← ⚠️ ② 批量发布，包在队里等
      → 用户态看到包

【DPDK】
  NIC DMA → rte_mbuf（hugepage mempool）
      → PMD 线程本来就在轮询（无中断、无调度）
      → rte_eth_rx_burst() 直接取           ← ✅ 无 ①②
      → 用户态看到包
```

| 消除的项 | AF_XDP 能做到吗 |
|---------|----------------|
| ① NAPI 调度等待 | **部分能**：开 busy poll（`SO_BUSY_POLL` + `SO_PREFER_BUSY_POLL`）后，`recvmsg()` 直接在调用上下文驱动 NAPI。但仍然是"用户态调一次 → 驱动跑一轮"的模型 |
| ② 批量发布等待 | **不能**。`xdp_do_flush()` 由 NAPI poll 结束触发（[01 篇](01-xdp-redirect.md)第一节），用户态无法提前拿到 |
| DMA 目标内存 | **能**（UMEM 可以放 hugepage，效果等同 mbuf pool） |
| 描述符环控制 | **不能**。环由驱动管理，AF_XDP 看到的是第二层（四个 ring） |

**还有一项非显式的：缓存热度。**
AF_XDP 路径上，内核已经碰过这个包一次（构造 `xdp_buff`、填 desc）；
DPDK 里用户态是第一个碰它的。在极小包、极致延迟场景，这点差异是可测的。

---

## 五、为什么 XDP 在工程上是更好的默认选择

| 优势 | 说明 |
|------|------|
| **与内核共存** | 一张卡：队列 0 给行情旁路，队列 1~N 仍走内核栈。**SSH、监控、NTP、组播管理流全部不受影响**——DPDK 做不到，它接管整张卡后你得再插一张卡跑管理流量 |
| **运维工具全部可用** | `ip link`、`ethtool -S`、`ss`、`tcpdump`（非旁路队列）、`bpftool`、`perf` 全在 |
| **渐进式引入** | 可以先只把一条流旁路掉，其余不动；出问题 `xdp-loader unload` 就恢复。**DPDK 的切换是"断网→绑驱动→起来"** |
| **不需要 hugepage 预留** | 内核不强求（建议用于 UMEM）。DPDK 必须预留 GB 级 hugepage，且要和 NUMA 节点对齐 |
| **网卡型号跟着内核走** | 用主线驱动即可，内核支持新卡你就支持。DPDK 要等对应 PMD 和版本匹配 |
| **故障爆炸半径小** | XDP 程序挂了只影响被 redirect 的流；PMD 挂了整张卡不通 |
| **开发迭代快** | BPF 可以热加载/热替换，verifier 帮你挡住大部分内存错误 |

---

## 六、性能数字：为什么不给，以及怎么自己测

**不给数字的理由**：XDP vs DPDK 的差距高度依赖

- 网卡型号与驱动（同为 10G，ixgbe 和 mlx5 差很多）
- 是否开 IOMMU（DMA 映射成本差异巨大）
- 包长（64 B vs 1500 B 的结论可以完全相反）
- 是否开 busy poll / 中断合并参数
- CPU 型号、NUMA 布局、是否超线程
- 内核版本与配置（`CONFIG_NET_RX_BUSY_POLL`、`CONFIG_PAGE_POOL_STATS` 等）

脱离这些条件引用的数字，在本机基本不可复现。

### 测量方法（三种，精度递增）

```bash
# ① 粗测：驱动/PPS 计数（吞吐，非延迟）
ethtool -S eth0 | grep -E 'rx_packets|tx_packets'
# 或 xdp-benchmark（xdp-tools 自带，可测 XDP 各动作的 Mpps）
xdp-bench drop eth0

# ② 中测：BPF 程序自身耗时（XDP 侧，不含用户态）
bpftool prog show id <ID>
#   run_cnt / run_time → 单包 BPF 平均耗时（纳秒级，内核直接测）

# ③ 精测：端到端，用硬件时间戳（唯一可信的延迟测量方式）
#   网卡侧：ethtool -T eth0 看是否支持 HW TX/RX timestamp
#   应用侧：SO_TIMESTAMPING 取 RX_HARDWARE 与用户态处理完成的差值
#
#   对照基线：先测"内核 socket + busy poll"，再测 AF_XDP，
#             两者同机同卡同包长对比才有意义
```

> **⚠️ 不要用 `gettimeofday()` 在用户态测单包延迟。**
> 你测到的是"从你看到包到你打完时间戳"，中间的调度、cache miss、
> 时钟读取本身的开销都会混进来。延迟测量请见
> [chapter-15/03](../../chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md)。

---

## 七、选型决策树

```
① 驱动支持 AF_XDP zero-copy 吗？
     └─ 不支持 ──→ 别用 AF_XDP。走内核栈 + busy poll
                   （或换网卡 / 上 DPDK）
     │
     支持
     ↓
② 这张网卡上还有别的流量要跑吗？（SSH/监控/其他业务/路由）
     └─ 有 ──→ XDP + AF_XDP（按队列旁路，其他队列走栈）
     │
     没有，这张卡就是给交易用的
     ↓
③ 团队能接受"整张卡从内核消失"的运维代价吗？
     ├─ 能，且需要确定性延迟（P99.9 是硬指标）──→ DPDK
     └─ 不能 / 无所谓 ──────────────────────────→ XDP + AF_XDP
     │
④ 需要 TCP 吗？
     └─ 需要 ──→ 别用 DPDK（要自己实现或用 F-Stack/mTCP 这类移植栈）
```

**一句话总结：**

> **要"跟内核共存"→ XDP；要"独占网卡换确定性"→ DPDK。**
> 这不是性能问题，是**所有权问题**。

---

## 八、混合架构（实际生产里最常见）

```
                    ┌──────────────────────────────────┐
   行情网卡（10G/25G）│  队列 0 ──→ AF_XDP ──→ 策略进程      │
                    │  队列 1~3 → 内核栈 → 组播加入/监控    │
                    └──────────────────────────────────┘
                                   ↑ 同一张卡，两种路径共存

                    ┌──────────────────────────────────┐
   交易网卡（给 DPDK） │  整卡被 PMD 接管 ──→ 下单进程        │
                    └──────────────────────────────────┘
                                   ↑ 单独一张卡，内核看不到

                    ┌──────────────────────────────────┐
   管理网卡（1G）     │  完全走内核栈：SSH / NTP / 监控       │
                    └──────────────────────────────────┘
```

| 流量 | 技术选择 | 理由 |
|------|---------|------|
| 行情接收（高 PPS、只读） | AF_XDP 或 DPDK | 旁路，不进协议栈 |
| 下单（低 PPS、要 TCP 或需要内核路由） | 内核 socket + busy poll | PPS 低，内核栈开销可接受；要 TCP 就只能走内核 |
| 管理/监控 | 内核栈 | 天然 |
| 组播加入（IGMP） | 内核栈 | **IGMP 必须由内核处理**——AF_XDP 旁路的队列收不到 IGMP，组播成员关系要由内核队列维护 |

> **⚠️ 一个实战陷阱**：如果你把**整个**网卡的所有队列都给了 AF_XDP，
> 内核就收不到 IGMP Query/Report，组播成员关系会超时被交换机剪掉。
> **务必留至少一个队列给内核栈处理控制面协议。**

---

## 九、常见误区（逐条澄清）

| 误区 | 事实 |
|------|------|
| "XDP 一定能替代 DPDK" | ❌ 需要确定性 P99.9 延迟、要独占网卡时，DPDK 的结构性优势无法用 XDP 补上 |
| "DPDK 一定比 XDP 快" | ⚠️ 非零拷贝的 AF_XDP 比 DPDK 慢很多；**零拷贝 AF_XDP 差距会显著缩小**，具体取决于硬件和包长——**必须自己测** |
| "XDP 不需要 CPU" | ❌ Native XDP 仍然跑在 NAPI poll 里，需要 CPU。只是比协议栈省，不是不需要 |
| "DPDK 不需要内核" | ⚠️ 需要 UIO/VFIO 内核模块做 IOMMU 映射，只是数据路径不进内核 |
| "AF_XDP 零拷贝用 page_pool" | ❌ 用 UMEM（`MEM_TYPE_XSK_BUFF_POOL`），内核 `xp_dma_map()` 独立映射。page_pool 只在 copy 模式那条路径上出现 |
| "cpumap 能省掉 sk_buff" | ❌ cpumap 在目标 CPU 上照样 `kmem_cache_alloc_bulk()` 分配 skb，终点是 `netif_receive_skb_list()`。它优化的是**分配位置**，不是**分配本身**。见 [01 篇](01-xdp-redirect.md) |
| "XDP 转发不会丢包" | ❌ devmap 的 `ndo_xdp_xmit()` 没发完就直接释放剩余帧，**不重排队**。见 [01 篇](01-xdp-redirect.md) |
| "开了 XDP 就没有中断了" | ❌ 默认仍走中断 + NAPI。要消除中断得开 busy poll |
| "DPDK 必须关闭超线程" | ⚠️ 不必须，但**轮询核的对端 SMT 兄弟核**如果被别的负载占用会引入抖动——建议隔离或不用 |
| "AF_XDP 是 DPDK 的替代品" | ⚠️ 是"**能跟内核共存的旁路方案**"，不是 DPDK 的等价替代。两者的适用边界不同 |

---

## HFT 要点

- **决策轴是所有权，不是速度**：要共存 → XDP；要独占换确定性 → DPDK。
- **先验证驱动支持 `XDP_ZEROCOPY`**（[chapter-06/01](../../chapter-06-af-xdp/notes/01-af-xdp.md)）。
  不支持就别用 AF_XDP，copy 模式不划算。
- **留一个队列给内核栈**：IGMP、ARP、SSH 都要走它。全旁路会导致组播成员关系超时。
- **延迟只有硬件时间戳测出来的才可信**，且必须用"内核 socket + busy poll"做对照基线。
- **DPDK 的隐藏成本不是开发，是运维**：整机网卡从内核消失，所有常规排障工具失效。
- **混合架构是常态**：行情接收旁路、下单走内核、管理走内核，
  用不同网卡/不同队列分开，别追求"全线旁路"。
- **不要引用别人的性能数字**。包长、驱动、IOMMU、CPU 型号任何一个不同，
  结论都可能反转。

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（4.8+ / v6.6） |
|------|-------------|---------------------|
| 旁路选项 | 只有 `PF_PACKET`（仍需拷贝一次到用户态） | XDP / AF_XDP 零拷贝、DPDK |
| 协议栈可绕过性 | 不可绕过 | **可按队列绕过** |
| 与内核共存 | 天然共存（因为没有旁路） | 需要显式设计（留队列 + `XDP_PASS` 回落） |
| 延迟优化手段 | 调中断合并、调缓冲区 | 旁路 + 轮询 + 内存池 + NUMA 绑定 |
| 内存管理 | 内核 skb + 页 | UMEM / hugepage mempool，用户态管理 |

---

## 代码自测

<details>
<summary><b>Q1：</b>团队想用 DPDK 替换现有的内核收包，理由是"DPDK 更快"。作为架构评审，你会问哪三个问题？</summary>

**不要先问"快多少"，先问这三个能决定成败的问题：**

**① 这张网卡上还有别的流量吗？**

DPDK 接管的是**整张卡**。接管后内核看不到它：
- SSH/监控/NTP 走不了（得再插一张管理网卡）
- 路由表、`ip`/`ethtool`/`tcpdump` 在该卡上全部失效
- IGMP 收不到 → 组播成员关系会被交换机剪掉

如果答案是"有"，AF_XDP 的**按队列旁路**是更合适的方案：
队列 0 旁路给策略，队列 1~N 照常走内核栈。

**② 需要 TCP 吗？**

DPDK 里没有内核协议栈。要用 TCP 得引入 F-Stack / mTCP / lwIP 这类移植栈，
那又是一整套需要调优和背锅的东西。
**行情（UDP 组播）可以旁路，下单（常要 TCP）通常不适合。**

**③ 团队能接受运维方式的改变吗？**

| 项 | 内核/XDP | DPDK |
|---|---------|------|
| 排障工具 | `ip` / `ethtool` / `ss` / `tcpdump` / `perf` / `dropwatch` | `dpdk-proc-info` / `testpmd` / `dpdk-telemetry` |
| 故障爆炸半径 | XDP 程序挂 → 该队列丢包 | PMD 挂 → **整张卡不通** |
| 部署 | 加载 BPF 程序 | 绑 VFIO + hugepage + NUMA 对齐 + 核隔离 |
| 升级耦合 | 跟内核（CO-RE 缓解） | 跟 DPDK 版本 + 内核 VFIO 接口 |

**如果这三个问题里有两个答不上来，默认选 XDP/AF_XDP。**
XDP 的上手成本远低于 DPDK，且可以随时 `xdp-loader unload` 回退；
DPDK 的切换是"断网 → 绑驱动 → 起来"的一次性动作。

**什么时候该坚持 DPDK**：需要 P99.9 的确定性延迟、能接受独占网卡、
团队有 DPDK 运维经验、且流量模型是纯 UDP 或自研协议。
</details>

<details>
<summary><b>Q2：</b>你把整张网卡的所有队列都配给了 AF_XDP，行情收得很好，但十几分钟后组播流断了。为什么？</summary>

**因为内核收不到 IGMP，组播成员关系超时被交换机剪掉了。**

AF_XDP 是**按队列**旁路的——被 redirect 进 socket 的包**不再进入内核协议栈**。
如果你把所有队列都给了 AF_XDP：

```
队列 0 ──→ AF_XDP（行情 UDP）      ✅ 正常
队列 1 ──→ AF_XDP                  ✅ 正常
...
队列 N ──→ AF_XDP                  ✅ 正常
                    ↓
        内核栈收不到任何包
                    ↓
        IGMP Query / Report 无人应答
                    ↓
        交换机判定"该端口无成员" → 停止转发组播
                    ↓
        行情断流 🎉
```

**同样会断的还有**：ARP（无法应答地址解析）、SSH（连不上）、
ICMP（ping 不通，监控告警）、NTP（时钟漂移）。

**解法：留至少一个队列给内核栈。**

```bash
# ① 确认队列数
ethtool -l eth0

# ② 用 flow steering 把行情流钉到 0 号队列给 AF_XDP
ethtool -N eth0 flow-type udp dst-port 12345 action 0

# ③ 其余队列的流量继续走内核栈
#    XDP 程序里非匹配包必须 XDP_PASS（不要写 flags=0！）
```

```c
/* XDP 程序：只把行情包送进 socket，其余全部交回内核栈 */
SEC("xdp")
int prog(struct xdp_md *ctx)
{
        if (is_market_data(ctx) && ctx->rx_queue_index == 0)
                return bpf_redirect_map(&xsks_map, ctx->rx_queue_index,
                                        XDP_PASS);   /* ⚠️ 第三个参数是 fallback */
        return XDP_PASS;
}
```

**⚠️ 顺带复习一个致命写法**（详见 [chapter-06/01](../../chapter-06-af-xdp/notes/01-af-xdp.md)第五节）：

```c
bpf_redirect_map(&xsks_map, idx, 0);   /* ❌ 0 = XDP_ABORTED = 静默丢包 */
```

flags 的低位是"lookup 失败时的返回值"（`include/linux/filter.h:1498`）。
写 0 会让所有没有对应 socket 的队列/协议被静默丢弃，
而 tcpdump 和 `ethtool -S` 都显示"一切正常"。
</details>

<details>
<summary><b>Q3：</b>有人说"DPDK 比 AF_XDP 快一倍"。你怎么判断这个说法在你的环境里成不成立？</summary>

**不接受现成数字，自己做对照实验。差距高度依赖下面这些变量，
任何一个不同结论都可能反转：**

| 变量 | 影响 |
|------|------|
| 网卡型号与驱动 | 同为 10G，ixgbe 与 mlx5 的 XDP 路径差异很大 |
| 是否开 IOMMU | DMA 映射成本差异巨大，能改变整个结论 |
| 包长 | 64 B 与 1500 B 的结论可以**完全相反**（小包看 per-packet 开销，大包看内存带宽） |
| busy poll / 中断合并 | AF_XDP 不开 busy poll 就多一次调度等待（[chapter-06/02](../../chapter-06-af-xdp/notes/02-af-xdp-lwn.md)第三节） |
| CPU 型号 / NUMA / 超线程 | 轮询核的兄弟核被占用会引入抖动 |
| 内核配置 | `CONFIG_NET_RX_BUSY_POLL` 决定 busy poll 能否生效 |

### 三段式测量

**① 先定基线（很重要，很多人省了这步）**

```
基线 = 内核 socket + SO_BUSY_POLL + SO_PREFER_BUSY_POLL
```
没有基线，你就不知道"AF_XDP 提升了多少"，更别说跟 DPDK 比。

**② 用硬件时间戳测端到端延迟（唯一可信）**

```bash
ethtool -T eth0      # 确认支持 hardware 时间戳
```

```c
/* 应用侧：SO_TIMESTAMPING 取硬件时间戳 */
int flags = SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING, &flags, sizeof(flags));
```

**延迟定义要写清楚**：从"网卡收到最后一个字节（HW RX timestamp）"
到"用户态策略函数被调用"的差值。

**⚠️ 不要用 `gettimeofday()` 在用户态掐表**——你测到的是
"从你看到包到你打完时间戳"，中间混入了调度、cache miss、时钟读取本身。
延迟测量详见 [chapter-15/03](../../chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md)。

**③ 分层定位，别只看总延迟**

```bash
# BPF 程序自身耗时（内核直接测，纳秒级）
bpftool prog show id <ID>       # run_time / run_cnt

# 驱动层 PPS 与丢包
ethtool -S eth0 | grep -E 'rx_packets|rx_missed|rx_dropped|xdp'

# XDP 各动作的吞吐上限
xdp-bench drop eth0
```

### 判断标准

| 结果 | 结论 |
|------|------|
| DPDK 优势 < 20% | **选 AF_XDP**——用可忽略的性能换运维能力和与内核共存 |
| DPDK 优势 20–50% | 看业务：如果 P99.9 是硬指标 → DPDK；否则 AF_XDP |
| DPDK 优势 > 50% | 八成是 AF_XDP 配置有问题（没开 busy poll / 降级到 copy 模式 / NUMA 不对），**先排查再下结论** |

**最后一条：如果 AF_XDP 的实测比预期慢 2–3 倍，
先查是不是静默降级到了 copy 模式**（[chapter-06/01](../../chapter-06-af-xdp/notes/01-af-xdp.md)第四节）：

```c
struct xdp_options opts;
socklen_t len = sizeof(opts);
getsockopt(fd, SOL_XDP, XDP_OPTIONS, &opts, &len);
/* opts.flags & XDP_OPTIONS_ZEROCOPY == 0 就是降级了 */
```
</details>

---

→ 本篇：[02 XDP vs DPDK](02-xdp-vs-dpdk.md)
→ 前一篇：[01 XDP_REDIRECT 的四个目的地](01-xdp-redirect.md)
→ 相关：[chapter-06 AF_XDP](../../chapter-06-af-xdp/) · [chapter-05 XDP 架构](../../chapter-05-xdp-architecture/) · [chapter-15 延迟测量](../../chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md) · [13-dpdk/](../../../13-dpdk/)
