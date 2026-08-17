## 2. 轮询与混合中断模式

---

### 一、传统中断 vs DPDK 轮询

| | **异步中断（传统驱动）** | **Poll Mode（DPDK）** |
|---|------------------------|----------------------|
| 触发 | 每包/批量 **硬中断** | lcore **主动轮询** 描述符环 |
| 开销 | **上下文切换**、中断延迟 | CPU **100% 占用**（该核） |
| 延迟 | ~50-100μs（中断 + softirq + 协议栈） | ~0.5-2μs（DMA + PMD poll） |
| 适用 | 通用 OS、低负载 | **高 PPS、低延迟** 数据面 |

**传统中断路径的延迟分解：**

```
NIC 收包 → DMA 写入内存 → 触发硬中断
  → ~1-5μs 中断响应延迟 (IRQ → do_IRQ)
  → 上下文切换 (kernel mode)
  → softirq NET_RX_SOFTIRO → net_rx_action()
  → NAPI poll → netif_receive_skb()
  → ip_rcv() → udp_rcv() → sock_queue_rcv_skb()
  → 唤醒用户态 (schedule) → 上下文切换 (user mode)
  → recvmsg() 返回数据
  总计：~50-100μs
```

**DPDK 轮询路径的延迟分解：**

```
NIC 收包 → DMA 写入预分配 mbuf → 置 DD 位
  → PMD 线程轮询 DD 位 → rte_eth_rx_burst() 取包
  → 用户态直接解析包头
  总计：~0.5-2μs
```

 内核 NAPI 对照 [12-kernel-networking](../../../../12-kernel-networking/) · [ULK Ch4 中断](../../../../16-linux-kernel-deep/chapter-04-interrupts-and-exceptions/)

---

### 二、UIO / VFIO — 用户态设备访问

DPDK 需要从用户态直接操作网卡寄存器，有两种框架：

| | **UIO (Userspace I/O)** | **VFIO (Virtual Function I/O)** |
|---|--------------------------|--------------------------------|
| 内核模块 | `igb_uio` (DPDK 自带) | `vfio-pci` (内核原生) |
| PCI BAR 映射 | `mmap()` 映射到用户态 | `mmap()` + IOMMU 保护 |
| 中断支持 | 通过 `/dev/uio` fd | 通过 eventfd / ioctl |
| IOMMU 隔离 | ❌ 无 | ✅ 有 — DMA 限制在授权范围 |
| 安全性 | 低 — 可 DMA 任意内存 | 高 — IOMMU 隔离 |
| 现代推荐 | 逐渐弃用 | ✅ 首选 |

```bash
# VFIO 绑定流程
modprobe vfio-pci
echo "0000:81:00.0" > /sys/bus/pci/drivers/ixgbe/unbind
echo "8086 1583" > /sys/bus/pci/drivers/vfio-pci/new_id

# 确认 IOMMU 已开启
# GRUB: intel_iommu=on iommu=pt
dmesg | grep -i iommu
# DMAR: IOMMU enabled
```

---

### 三、混合中断轮询模式

**动机：** 纯轮询 **费电、空转 CPU** — 空闲时希望 **休眠 + 中断唤醒**。

| 组件 | 作用 |
|------|------|
| **UIO / VFIO** | 用户态映射设备，注册中断 |
| **epoll** | 收包线程 **阻塞等待** `/dev/uig` 或 eventfd 可读 |
| **l3fwd-power** | 官方示例：连续 N 次空 poll → **使能中断 + sleep**；中断到达 → **关中断 + 继续 poll** |

**l3fwd-power 状态机：**

```
                空闲检测
POLL ─────────────────→ ENABLE_IRQ + sleep
(轮询模式)                (休眠模式)
    ↑                        │
    └──── 中断唤醒 ──────────┘
         DISABLE_IRQ
```

**代价：**

- **首包延迟** — 从休眠到唤醒的路径（~10-50μs，远大于纯轮询的 ~1μs）
- 需适当 **加大 mbuf/环深度**，避免唤醒前 **溢出丢包**

**HFT：** 热路径行情核通常 **禁用** 此模式 — 求 **确定性延迟** 而非省电。但 **非热路径**（如管理面、备用链路）可启用省电。

---

### 四、与 Ch6 的衔接

- 轮询仍是对 **描述符环** 的批量检查 — [Ch6 §3 DMA 环](../../chapter-06-pcie-packet-io/notes/section-3-DMA描述符环形队列.md)
- 混合模式改变的是 **「何时 stop polling」**，不改变 burst 处理模型（→ §3）

 repo PMD 实验 stub：[chapter-03-PMD与轮询模式.md](../../chapter-03-parallel-computing/)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. I/O 深度优化](./section-3-IO性能深度优化.md)
