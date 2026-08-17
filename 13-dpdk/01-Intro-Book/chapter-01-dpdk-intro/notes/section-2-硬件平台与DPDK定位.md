## 2. 主流包处理硬件平台与 DPDK 定位

---

### 一、三类硬件方向

支撑 **网络数据包处理** 的主流平台：

| 方向 | 特点 | 典型产品 | HFT 场景 |
|------|------|----------|----------|
| **硬件加速器** | 专用 ASIC/FPGA — 固定功能、极高吞吐 | Mellanox BlueField、Solarflare | FPGA 做 Layer-1 解码 / 订单簿维护 |
| **网络处理器 (NPU)** | 可编程包处理芯片 — 灵活但开发门槛高 | Broadcom、Marvell | 交易所撮合引擎前端 |
| **多核处理器 (MP)** | **通用 IA（Intel Architecture）** — 灵活、生态大 | Xeon Scalable、AMD EPYC | 共置机策略引擎 + 行情网关 |

---

### 二、DPDK 的选择

DPDK 基于 **通用 IA 多核**，用 **软件** 演绎数据面：

- **不依赖** 专用 NPU/加速器才能跑满线速
- 通过 **工程最佳实践** 释放 IA 平台吞吐（→ [section-3](./section-3-性能最佳实践.md)）
- **核心洞察：** CPU 计算速度远超 I/O — 问题不在算力，而在 **数据搬运路径**（中断、拷贝、cache miss）

**HFT 含义：** 共置机多为 **x86 多核** — DPDK 与策略代码 **同机、绑核、同 NUMA** 是常见落地形态。FPGA 负责 PHY 层 + 预处理，DPDK 负责组播行情接收 + 策略信号发出，形成 **FPGA → DPDK → Strategy** 三级流水线。

---

### 三、与内核栈的关系

| 路径 | 谁处理包 | 延迟构成 |
|------|----------|----------|
| **内核栈** | 硬中断 → softirq (NET_RX) → NAPI → `netif_receive_skb()` → sk_buff → socket queue → `recvmsg()` | 中断延迟 (~50-100μs) + 上下文切换 + sk_buff 分配/拷贝 + 协议栈遍历 |
| **DPDK** | 用户态 **PMD 轮询** → mbuf — **绕过** 上述路径 | 轮询延迟 (~0-2μs) + DMA 写入 + cache miss |

**内核栈被绕过的具体环节：**

```
传统路径：
  NIC → DMA → RAM → 硬中断 → softirq → NAPI poll → netif_receive_skb
       → ip_rcv → udp_rcv → sock_queue_rcv_skb → recvmsg → 用户态

DPDK 路径：
  NIC → DMA → RAM（预分配 mbuf）→ PMD 轮询 DD 位 → rte_eth_rx_burst() → 用户态
```

- **无中断** — PMD 线程 100% 轮询，不触发 `IRQ → softirq`
- **无 sk_buff** — DPDK 用自己的 `rte_mbuf`，避免内核分配/释放开销
- **无协议栈** — 用户态直接解析 L2/L3/L4 头
- **无系统调用** — 收发包不经过 `recvmsg()`/`sendmsg()`，零 `syscall` 开销

 对照：[12-kernel-networking](../../../../12-kernel-networking/) · [chapter-04 零拷贝](../../chapter-04-synchronization)

---

### 四、DPDK 架构总览

```
┌─────────────────────────────────────────┐
│              用户态应用层                  │
│  (L3fwd / 行情网关 / 策略引擎)            │
├─────────────────────────────────────────┤
│  DPDK 库层                                │
│  rte_ethdev  rte_mbuf  rte_mempool       │
│  rte_hash    rte_lpm  rte_ring           │
│  rte_eal     rte_lcore  rte_cycles       │
├─────────────────────────────────────────┤
│  PMD 驱动层（用户态）                      │
│  ixgbe  i40e  ice  mlx5  virtio          │
├──────────┬──────────┬───────────────────┤
│  UIO     │  VFIO    │  bifurcated       │
│ (igb_uio)│ (VFIO-PCI)│ (内核 + 用户态)   │
├──────────┴──────────┴───────────────────┤
│           Linux 内核（仅做 PCI 映射）      │
├─────────────────────────────────────────┤
│              硬件 NIC                     │
└─────────────────────────────────────────┘
```

- **UIO（Userspace I/O）：** 内核仅映射 PCI BAR 和注册中断，不做包处理。`igb_uio` 是 DPDK 传统 UIO 模块
- **VFIO（Virtual Function I/O）：** 比 UIO 更安全，支持 IOMMU 隔离，是现代 DPDK 首选。`vfio-pci` 绑定网卡后，用户态可直接访问设备寄存器
- **bifurcated driver：** 内核和用户态共享设备（如 mlx5），内核仍管理控制面，数据面走用户态

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 最佳实践](./section-3-性能最佳实践.md)
