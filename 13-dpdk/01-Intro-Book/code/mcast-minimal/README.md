# 组播行情最小工程

> DPDK 最小可运行示例：绑定网卡 → 收 UDP 组播 → 解析行情头 → 输出分位延迟。
>
> 两个可执行文件**用同一套分位统计口径**，可以直接相减得出"旁路省了多少"。

## 目录结构

```
mcast-minimal/
├── README.md
├── Makefile
└── src/
    ├── hist.h                 # 分位直方图（header-only，两版共用）
    ├── main.c                 # DPDK 旁路版
    └── mcast_socket_ref.c     # 内核协议栈对照版（无 DPDK 依赖）
```

## 两个版本的对照

| | `mcast_socket_ref`（内核栈） | `mcast_minimal`（DPDK 旁路） |
|---|---|---|
| 依赖 | 无，任何 Linux 可编译 | DPDK 22.11/23.11 LTS + 大页 + 网卡绑定 |
| 收包调用 | `recvmmsg()` | `rte_eth_rx_burst()` |
| 系统调用 | 每批 1 次 | **0 次** |
| 数据拷贝 | 内核 → 用户态 1 次 | **0 次**（DMA 直写 mbuf） |
| 协议栈 | IP/UDP 解析、组播复制、socket 匹配 | **无**，自己解析以太网帧 |
| IGMP | 内核自动发 Membership Report | **必须自己处理**（见下） |
| 网卡占用 | 与内核共存 | **独占**该端口 |

**延迟口径对照：**

```
socket 端到端 ≈ hist_recv（系统调用+拷贝） + hist_burst（批次内位置）
DPDK   端到端 ≈ hist_burst（批次内位置）          ← 没有 recvmmsg 这一项
```

两者的 `hist_burst` 同口径可直接比；差值中属于协议栈和拷贝的部分，
就是 `mcast_socket_ref` 的 `hist_recv`。

## 编译

```bash
cd 13-dpdk/01-Intro-Book/code/mcast-minimal
make                      # 有 DPDK 编两个，没 DPDK 自动只编对照组
make mcast_socket_ref     # 只编对照组
```

对照组在任何 Linux 上都能编（含树莓派），**不需要 DPDK、大页、网卡绑定**。
建议先把它跑通拿到 baseline 数字，再折腾 DPDK 环境。

## 运行：内核栈对照版

```bash
# 需要一个能发 UDP 组播的源；本机自发自收可用：
#   sudo ip route add 224.0.0.0/4 dev eth0     # 组播走指定网卡
#   socat -u - UDP-DATAGRAM:224.1.2.3:12345,ip-add-membership=224.1.2.3:eth0

./mcast_socket_ref -g 224.1.2.3 -p 12345 -i 192.168.1.10 -v 32
#   -i 本机接口 IP（加入组播用，多网卡时必填）
#   -v 批量大小，对应 DPDK 的 BURST_SIZE
#   -b SO_BUSY_POLL 微秒数（>0 时该核会 100% 占用换低延迟）
```

## 运行：DPDK 版

```bash
# 1) 大页
echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mkdir -p /mnt/huge && mount -t hugetlbfs nodev /mnt/huge

# 2) 绑定网卡到 vfio-pci（网卡会被内核放弃管理，SSH 别走这张卡！）
modprobe vfio-pci
dpdk-devbind.py --bind=vfio-pci 0000:01:00.0

# 3) 运行（-- 之前是 EAL 参数，之后是程序参数）
./mcast_minimal -l 2 -n 4 -- -g 224.1.2.3 -p 12345
#     -l 2 表示跑在 lcore 2 上；生产环境要 isolcpus 把它隔离出来
```

### ⚠️ 收不到包？先查 IGMP

旁路之后**内核不再管理这张网卡，IGMP Membership Report 不会自动发出**。
交换机靠 IGMP snooping 决定是否转发，没学到就一个包都不给你 ——
程序看起来是"rx_burst 一直返回 0"，很容易误判成代码错误。

三选一：

1. 交换机上配**静态组播转发**（static mrouter port / static group）→ 最省事
2. DPDK 应用自己构造并发送 IGMPv2/v3 Membership Report
3. 交换机端口配成 flood（不推荐，流量放大几十倍）

排查顺序：`ethtool -S` 看网卡计数 → 计数不涨是交换机/IGMP 问题；
计数涨但程序收不到才是用户态代码问题。

详见 [12.5/chapter-02/notes/05-multicast-rx-path.md](../../../../12.5-modern-networking/chapter-02-napi-rx-path/notes/05-multicast-rx-path.md)

## 输出解读

两个版本的收尾都会打印：

```
每包解析延迟 (ns)  (样本 10000000, 溢出 0)
  p50      p99      p999     p9999    max      mean
  820      1950     6400     21000    128000   910.4
```

- **看 p999，不看均值。** 决定策略生死的是尾延迟
- 样本 < 100 万时 p999/p9999 无统计意义，程序会警告
- 有"溢出"说明样本超出直方图量程，需调大 `HIST_TICKS_PER_BUCKET`
- `hist_oh` 是时钟开销基线（两版都有），严格报告时从其余两项中扣除

丢包必须盯着 —— **组播 UDP 没有重传**：

| 版本 | 丢包指标 | 含义 |
|---|---|---|
| DPDK | `imissed` | 网卡收不进来（PCIe/队列满） |
| DPDK | `rx_nombuf` | mbuf 池耗尽（用户态消费太慢） |
| 对照 | `SO_RXQ_OVFL` | socket 接收队列溢出 |

真实系统必须做**序列号 gap 检测 + 独立的 TCP 补单通道**。

## 延伸

- 批量大小实验：改 `-v` / `BURST_SIZE`，观察 `hist_burst` 的 p999。
  批量越大吞吐越高，但批次内后包的队头等待越长 —— 这就是"吞吐 vs 尾延迟"的取舍
- 加解码：在 `parse_packet()` 返回后直接读 `v->payload`，接 MoldUDP64 / ITCH
- 延迟测量方法论：[12.5/chapter-15/notes/03-latency-measurement.md](../../../../12.5-modern-networking/chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md)
- 组播行情笔记：[../notes/chapter-05-组播行情接入.md](../notes/chapter-05-组播行情接入.md)
