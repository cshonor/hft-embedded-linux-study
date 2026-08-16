# HFT 场景 03：内核旁路与 Wireshark 局限

> [总览](./00-overview.md) · [HFT 场景 02：NIC offload](./02-nic-offload-impact.md) · [HFT 模块：DPDK](../../13-dpdk/)

**核心问题**：HFT 系统常用 DPDK/AF_XDP 绕过内核协议栈，直接从网卡收包到用户态。Wireshark 依赖内核协议栈（`AF_PACKET` socket），**完全看不到内核旁路的流量**。

## 1. Wireshark 为什么抓不到

```
传统内核路径（Wireshark 可见）：
  网卡 RX → 内核驱动 → 协议栈 → AF_PACKET socket → Wireshark/tcpdump

DPDK 路径（Wireshark 不可见）：
  网卡 RX → DPDK PMD 驱动 → 用户态内存（rte_ring）→ 应用
  ↑ 内核完全不知道有包经过

AF_XDP 路径（Wireshark 不可见）：
  网卡 RX → XDP 程序 → AF_XDP socket → 用户态
  ↑ 包在 XDP 层就被重定向，不到达协议栈
```

## 2. 内核旁路技术对比

| 技术 | 原理 | Wireshark 能否抓包 | 延迟 |
|------|------|-------------------|------|
| **DPDK** | 用户态 PMD 驱动，UIO/VFIO 绕过内核 | 不能 | <5μs |
| **AF_XDP** | eBPF + UMEM，XDP 层重定向 | 不能（默认） | 5–20μs |
| **netmap** | 轻量内核旁路 | 不能 | ~10μs |
| **Solarflare OpenOnload** | 内核旁路 + Socket 兼容 | 部分（可 fallback 到内核） | <5μs |
| **传统 Socket** | 内核协议栈 | 能 | 50–200μs |

## 3. DPDK 场景的抓包替代方案

### 方案 A：DPDK 内置抓包（rte_pcap）

```bash
# DPDK 18.05+ 支持将收到的包写为 pcap
# 在应用代码中调用 rte_pcap_dump()
# 或使用 dpdk-pdump 工具

# 启动 pdump server
dpdk-pdump --pdump 'port=0,queue=*,rx-dev=/tmp/dpdk_rx.pcap,tx-dev=/tmp/dpdk_tx.pcap'

# 分析生成的 pcap
wireshark /tmp/dpdk_rx.pcap
```

| 优点 | 缺点 |
|------|------|
| 可抓到 DPDK 旁路流量 | 需要应用支持 pdump 框架 |
| 不影响数据路径 | 抓包会增加 CPU 开销 |
| 生成标准 pcap | 时间戳精度依赖 PMD 轮询周期 |

### 方案 B：物理层分光/镜像（最可靠）

```
交换机端口镜像（SPAN/RSPAN）→ 抓包机 → Wireshark
            或
光纤分光器（TAP）→ 抓包机 → Wireshark
```

| 优点 | 缺点 |
|------|------|
| 不影响生产系统任何性能 | 需要额外硬件（分光器/抓包机） |
| 看到真实线缆包 | 交换机镜像可能丢包 |
| 适用于所有旁路技术 | 成本高 |

### 方案 C：虚拟交换机抓包（容器/VM 环境）

```bash
# 如果 DPDK 应用跑在 VM 中，通过 Open vSwitch 抓包
ovs-tcpdump -i vnet0 -w vm_traffic.pcap

# 或在宿主机上抓物理网卡（但看不到 DPDK 到 VF 的流量）
tcpdump -nni eth0 -w host_traffic.pcap
```

## 4. AF_XDP 场景的抓包方案

### 方案 A：XDP 分流（部分流量到内核）

```c
// 在 XDP 程序中，将非关键流量放回内核
// 关键交易流量到 AF_XDP socket
if (is_trade_packet(ctx))
    return XDP_REDIRECT;  // 到 AF_XDP，Wireshark 看不到
else
    return XDP_PASS;      // 到内核，Wireshark 可见
```

### 方案 B：tcpdump 在 AF_XDP 启动前抓

```bash
# 在 AF_XDP 程序启动前开始抓包
# AF_XDP 不会阻止已有的 AF_PACKET socket
# 但新流量会被 XDP 重定向，不再到达 AF_PACKET
sudo tcpdump -nni eth0 -w before_xdp.pcap
# 然后启动 AF_XDP 程序
```

## 5. Solarflare OpenOnload 场景

OpenOnload 是特殊案例——它尝试旁路内核，但可以 fallback。

```bash
# 检查 Onload 是否旁路
onload_stackdump | grep -i bypass

# 强制走内核（让 Wireshark 可见，但延迟增加）
EF_FORCE_KERNEL=1 ./trading_app

# 或在 Onload 配置中启用 packet capture
export EF_PACKET_CAPTURE=1
```

## 6. HFT 抓包策略决策树

```
交易系统用什么网络方案？
│
├─ 传统 Socket（setsockopt）
│   └─ tcpdump/Wireshark 直接可用 ✓
│
├─ DPDK
│   ├─ 能改代码？→ dpdk-pdump
│   ├─ 有交换机？→ SPAN/TAP 镜像
│   └─ 都不行？→ 只能分析应用层日志
│
├─ AF_XDP
│   ├─ 能改 XDP 程序？→ 分流部分到内核
│   └─ 不能？→ SPAN/TAP 镜像
│
└─ OpenOnload
    └─ EF_FORCE_KERNEL=1 或 EF_PACKET_CAPTURE=1
```

## 7. HFT 自测题

1. DPDK 应用收到的包，内核协议栈知道吗？tcpdump 能抓到吗？
2. AF_XDP 程序返回 `XDP_REDIRECT` 和 `XDP_PASS`，Wireshark 分别能看到什么？
3. 你在 HFT 生产环境用 SPAN 镜像抓包，发现 RTT 比应用日志记录的高 10μs。可能原因？
4. OpenOnload 的 `EF_FORCE_KERNEL=1` 会带来什么副作用？为什么 HFT 生产环境不用它？

## 交叉引用

- [HFT 场景 02：NIC offload](./02-nic-offload-impact.md)
- [HFT 场景 05：eBPF 对比](./05-ebpf-vs-wireshark.md)
- [HFT 模块：DPDK](../../13-dpdk/)
- [HFT 模块：内核网络](../../12-kernel-networking/)
- [HFT 模块：BPF 可观测性](../../06.7-bpf-observability/)
