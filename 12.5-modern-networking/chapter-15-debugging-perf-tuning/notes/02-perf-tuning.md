# 15-02 — 网络性能调优：网卡/内核/拓扑三层清单（v6.6 修订版）

> **Bootlin 课程模块：** Network Performance Tuning
> **对应 Rosen:** Ch14
> **关联核验:** `net/core/dev.c`（RPS/backlog）、`net/ipv4/tcp_output.c`（TSQ/EDT）、chapter-13/14 的源码结论

## 章节导航

| 上一篇 | 本篇 | 下一篇 |
|---|---|---|
| [15-01 调试工具链](01-debugging.md) | **15-02 性能调优** | [15-03 延迟量测](03-latency-measurement.md) |

## 本节讲什么

调优的本质是**把每一跳的固定成本压掉或摊薄**。本篇按"网卡层 → 内核层 → 拓扑层"组织一份每项都带"为什么"的清单，并修正旧版两处流传甚广的错误依据：**"关 GRO 减少合并延迟"和"关 TSO 降低发送延迟"**——这两条在本仓库 [14-01](../../chapter-14-tcp-udp-internals/notes/01-tcp-internals.md) 已经用源码证伪（GRO 没有等待窗口、TSO 不等凑包），调优决策应该基于正确的机制模型。最后给出树莓派 5 的实测修正（旧版写的 BCM2712 网卡是错的——那是 SoC，网卡在 RP1 南桥上）。

## 要点（先记住结论）

1. **低延迟的第一杠杆是中断合并，不是 offload**：`rx-usecs` 是收包延迟里最大的可调项（每微秒合并窗 = 直接加到尾延迟上）；GRO/TSO 的开关对单条小消息流的影响测不出来。
2. **关 GRO/TSO 的正确理由是"可控性与抖动"，不是"减少延迟"**：offload 路径的批量行为（TSO 分段 microburst、GRO 合并包的增量校验）在个别场景引入抖动，追求行为可预测时才关——并且要接受吞吐和 CPU 的代价。
3. **sysctl 里对延迟真正有意义的是 backlog 与 busy_poll 一族**；`rmem_max/wmem_max` 只在 buffer 不足（丢包/限速证据出现）时才调大——盲目调大伤 cache 局部性。
4. **qdisc 的选择按流量画像**：交易口 `pfifo` 小限深（排队可见可控）；行情转发口 `fq`（EDT pacing 的执行者，[14-01](../../chapter-14-tcp-udp-internals/notes/01-tcp-internals.md)）；默认 `fq_codel` 对 HFT 无益（AQM 是为 bufferbloat 设计的）。
5. **CPU 隔离的现代组合是 `nohz_full` + `rcu_nocbs` + IRQ 亲和 + cgroup cpuset**：isolcpus 仍可用但粒度粗；**irqbalance 必须停**——否则亲和性设置会被周期性改掉。
6. **NUMA 错位是隐形的百 ns 级税**：网卡 PCI 所在节点、中断目标核、应用线程三者的 NUMA 一致性要用 `numa_node` 文件实测，不能假设。

## 一、网卡层

### 中断合并：延迟 vs CPU 的直接交换

```bash
ethtool -c eth0                        # 看当前 coalescing
ethtool -C eth0 rx-usecs 0 rx-frames 1 # 每包一中断（最低延迟，CPU 换的）
ethtool -C eth0 adaptive-rx off        # 关掉自适应（否则驱动会自己改回去）
```

机制：硬件攒 `rx-usecs` 微秒或 `rx-frames` 个包才触发一次 MSI-X。**合并窗直接加在尾延迟上**（包到了但要等窗结束）。HFT 交易口用 0/1；行情口按 PPS 算一下中断风暴是否可承受（每秒 10 万中断 ≈ 单核 5-10% 只做中断）。

### 队列与 RSS

```bash
ethtool -l eth0 && ethtool -L eth0 combined 4    # 队列数 = 预期的并行 softirq 数
ethtool -X eth0 equal 4                          # indirection table 均匀分发
# 组播流量 RSS 失效（同 4-tuple）→ ntuple 规则按组播组分流：
ethtool -N eth0 flow-type udp4 dst-ip 239.1.1.1 action 1   # 该组 → RX 队列 1
```

（RSS 对组播失效的机理与 ntuple 方案见 [13-01](../../chapter-13-zerocopy-highperf/notes/01-scaling.md)。）

### offload 取舍（修订版，与 ch14 结论统一）

| offload | 建议 | 依据 |
|---|---|---|
| GRO | 交易口 **off**、转发口 **on** | GRO 无等待窗口（[14-02](../../chapter-14-tcp-udp-internals/notes/02-udp-gro.md)），关它不是为了延迟——是让每包独立走完路径、行为可预测；转发口开着 PPS 能力 2-3x |
| TSO | **on** | 不等凑包（[14-01](../../chapter-14-tcp-udp-internals/notes/01-tcp-internals.md)），关它只损失吞吐；真问题是 TSO 段的 microburst，由 fq pacing 削 |
| checksum（rx/tx） | **永远 on** | 硬件校验免费；关掉是纯 CPU 浪费（除非在抓校验错误的 bug） |
| GSO | **on** | 软件路径的 TSO 延伸，无理由关 |

## 二、内核层（sysctl）

```bash
# --- 对延迟有直接意义的 ---
net.core.netdev_max_backlog = 10000    # backlog 满会丢包（softnet_stat 第2列），但调大 = 允许更多排队
net.core.busy_read = 50                # μs；读路径 busy poll 窗口
net.core.busy_poll = 50                # μs；poll/epoll 路径 busy poll 窗口

# --- buffer：有证据（丢包/限速）才调 ---
net.core.rmem_max = 33554432           # 大流量转发才需要；交易口默认即够
net.core.wmem_max = 33554432
net.ipv4.tcp_rmem / tcp_wmem           # TCP 自动调节的 max 列

# --- 对照诊断，不是调优 ---
net.ipv4.tcp_no_metrics_save = 1       # 不让历史 RTT/cwnd 影响新连接（一致性）
# net.ipv4.tcp_low_latency：已移除的 no-op，别写进配置
```

busy poll 家族的语义（chapter-12 有 io_uring 版对照）：应用阻塞在 `recvmsg`/`epoll` 时，内核在这几十微秒里**主动轮询驱动收包队列**而不是睡等中断——用 CPU 换"中断唤醒 + cache 冷却"那两跳的延迟。

### qdisc

```bash
# 交易口：短队列，排队行为可见可控
tc qdisc replace dev eth0 root pfifo limit 256

# 行情转发口：fq（TCP EDT pacing 的执行者）
tc qdisc replace dev eth1 root fq

# 检查当前 qdisc 与排队：
tc -s qdisc show dev eth0              # 看 backlog/drops 计数
```

`fq_codel`（很多发行版默认）的 AQM 逻辑是为交互流量抗 bufferbloat 的，HFT 场景要么 `pfifo`（显式限深）要么 `fq`（EDT 配套），没有第三选项。

## 三、拓扑层：CPU 隔离与 NUMA

### IRQ 亲和（收包核的前提）

```bash
grep eth0 /proc/interrupts                     # 每个 RX 队列一个 MSI-X 向量
cat /proc/irq/<N>/smp_affinity_list
echo 2 > /proc/irq/<N>/smp_affinity_list      # RX-0 → CPU2
systemctl stop irqbalance                      # ⚠️ 必停，否则会被改回
```

### 内核参数组合（grub/cmdline）

```
nohz_full=2-5      # 这些核不做周期 tick（每秒少几百次时钟中断）
rcu_nocbs=2-5      # RCU 回调挪到别的核（否则隔离核被 RCU 唤醒）
# isolcpus=2-5     # 粗粒度老办法；现代等价物是 cgroup cpuset isolated partition
```

cgroup v2 的 cpuset 隔离分区（6.x）：

```bash
# 把 CPU 2-5 从调度器整体摘出（比 isolcpus 细，可在线调整）
echo "2-5" > /sys/fs/cgroup/machine.slice/cpuset.cpus
echo root > /sys/fs/cgroup/machine.slice/cpuset.cpus.partition
```

### NUMA 一致性三查

```bash
cat /sys/bus/pci/devices/0000:xx:00.0/numa_node   # 网卡在哪个节点
cat /sys/class/net/eth0/device/numa_node           # 同上（另一条路）
numactl --cpunodebind=0 --membind=0 ./trader      # 应用绑同节点
```

错位代价：跨 NUMA 的 DMA/中断/内存访问每跳加 ~100-140ns 且流量过 interconnect——对微秒级路径是可见税项。

## 四、HFT 综合调优清单（修订版：每项给真实理由）

| 项目 | 设置 | 真实目的 |
|------|------|------|
| 中断合并 | rx-usecs 0/1，adaptive off | **消灭合并窗延迟**（第一杠杆） |
| GRO | 交易口 off | 行为可预测、逐包独立，**不是**省"合并延迟"（不存在） |
| TSO | on | 关掉无收益（不等凑包），只损失吞吐 |
| RSS/ntuple | 按流分队列 | 组播下 RSS 失效，用 ntuple 按组分流（13-01） |
| RPS/RFS | off（有 RSS 时） | 软件 steering 的 IPI/cache miss 是纯开销（13-01） |
| SO_BUSY_POLL | 50-100μs | 换掉中断唤醒两跳 |
| CPU 隔离 | nohz_full + rcu_nocbs + cpuset partition | 清除隔离核上的定时器/RCU 唤醒 |
| IRQ 亲和 | RX 队列 → 隔离核，irqbalance 停 | softirq 与应用同核（13-01 的全链路同核范式） |
| NUMA | 网卡/中断/应用同节点 | 消跨节点 ~100ns/跳 |
| QDisc | 交易口 pfifo(256)；转发口 fq | 排队可见可控 / EDT pacing 配套 |
| clocksource | tsc（确认） | 时间读数的纳秒级前提（15-03 详述） |

## 五、树莓派 5 修正（重要事实纠错）

旧版写的"BCM2712 网卡"是错的——BCM2712 是 SoC；**网口在 RP1 南桥上，是 Cadence GEM 控制器 + Broadcom BCM54213 PHY，驱动为 `macb`**（不是 Pi 4 的 bcmgenet）。上游主线对 RP1 网口的支持 2025 年才合入（6.12+ 陆续补齐）；树莓派官方内核早已可用。

实测清单（装好系统先跑，别抄结论）：

```bash
ethtool -i eth0                     # 确认 driver=macb
ethtool -k eth0                     # offload 实际能力（GEM 的 checksum/散射列表）
ethtool -l eth0                     # 队列数——单队列的话 RSS/IRQ 分流全部免谈
ethtool -c eth0                     # 中断合并可调与否
dmesg | grep -i -E "macb|xdp"       # XDP 支持迹象（macb 的 XDP 支持较新且不完整）
cat /sys/bus/pci/devices/*/numa_node # 单节点系统，此项无意义（只为验证习惯）
```

预期画像：单/少队列 GbE、offload 集比服务器网卡窄、XDP 只在较新内核可用（generic 模式随时可用但性能意义有限——chapter-05 已讲 native vs generic 的差距）。**Pi 5 是学习/验证平台，不是低延迟参照系**——清单里"中断合并清零 + busy poll + 单核隔离"这套最小组合可以练手，RSS/多队列范式练不了。

## 衔接

调优把成本压到位之后，需要验证"到底多快"。下一篇 [15-03 延迟量测](03-latency-measurement.md)（已是完整版）覆盖 SO_TIMESTAMPING 硬件时间戳、时钟域对齐与分位数统计——量测方法论是一切调优的可信度来源。

## 代码自测

<details>
<summary>Q1：为什么说中断合并是低延迟的第一杠杆，而 GRO/TSO 不是？</summary>

量级和机制都不同：中断合并窗（rx-usecs，通常 25-125μs 的默认值）**直接加在每包尾延迟上**且可调到 0——是微秒级的第一大项；GRO 没有等待窗口（NAPI 批内机会主义合并）、TSO 不等凑包（发送时刻由 tcp_push 决定）——两者对"单条小消息何时到达/发出"的影响在纳秒级且测不出显著差。把可调项按对目标指标的影响排序，是调优清单的第一设计原则。
</details>

<details>
<summary>Q2：fq_codel 默认留着不行吗？它是"更好"的 qdisc 啊。</summary>

"更好"是有对象的：fq_codel 的 AQM（主动队列管理）为对抗 bufferbloat 设计——在深队列里智能丢包/标记，保护**交互流**不被**批量流**的积压伤害。HFT 交易口的问题是"任何排队都不可接受"——正确解是 `pfifo limit 256` 把队列深度显式钉死（排队即丢，丢即暴露），AQM 的智能反而让排队"安静地"发生。转发口则要 fq 而不是 fq_codel——EDT pacing（14-01）的执行者是 fq。
</details>

<details>
<summary>Q3：netdev_max_backlog 调大和调小分别适合什么？HFT 该选哪边？</summary>

调大：吞吐优先——backlog 是 RPS/softirq 的缓冲（`enqueue_to_backlog`，dev.c:4788），满则丢（softnet_stat 第 2 列），大 backlog 吸收突发不丢包，代价是排队延迟变长。调小：延迟优先——早丢早暴露，排队不会悄悄积累。HFT 交易口倾向**默认甚至更小 + 严格监控第 2 列**（任何 drop 都是 capacity 问题的信号，而不是该调大 backlog 的信号）；转发口（丢包代价高、有应用层重整）才往上调。
</details>

<details>
<summary>Q4：nohz_full 和 rcu_nocbs 为什么必须成对出现？</summary>

nohz_full 让隔离核脱离周期 tick（消除定时器中断噪声），但**RCU 回调**（延后释放的回调，如 RCU 宽限期结束后的释放工作）默认会在任意核上执行——隔离核一旦碰上 RCU 回调照样被唤醒，nohz 白做。rcu_nocbs 把这些核的 RCU 回调卸载（offload）给专用 rcuo 线程跑在别的核上。单用 nohz_full 的典型症状：隔离核上 `perf stat` 仍能看到零星的内核活动，找半天发现是 RCU。
</details>

<details>
<summary>Q5：树莓派 5 上按服务器清单照搬调优，哪几项注定无效？</summary>

①RSS/多队列分流——macb 单队列（或极少队列），`ethtool -X` 无从谈起；②IRQ per-queue 亲和的多核范式——没有多个 RX 向量可绑；③NUMA 一致性——单节点系统无跨节点问题；④EDT pacing 的收益——GbE 线速下 TSO 段的 microburst 影响和 fq 红黑树成本基本对冲。能练的：中断合并清零、busy poll、单核隔离（nohz_full+cpuset）、SO_TIMESTAMPING 量测闭环（15-03）——方法论部分和服务器完全同构，这正是 Pi 5 作为学习平台的价值。
</details>
