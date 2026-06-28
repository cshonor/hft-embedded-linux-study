## 4. Virtio 网络驱动设计与架构对比

---

### 一、Linux 内核驱动（分层）

```
┌─────────────────────────┐
│ 网络设备层 (net_device)  │  ← 标准内核网络栈接口
├─────────────────────────┤
│ Virtio 虚拟队列层        │  ← virtqueue 抽象
├─────────────────────────┤
│ PCI-e / MMIO 设备层      │  ← 寄存器、MSI-X
└─────────────────────────┘
```

**目标：** 与 **通用 Linux 网络栈** 无缝集成 — sk_buff、NAPI、iptables 等。

→ [13-LKN](../../../13-Linux-Kernel-Networking/) · 对照 DPDK **旁路** [Ch4 stub](../chapter-04-零拷贝与用户态旁路.md)

---

### 二、DPDK 用户态驱动

**目标：** **极致包处理** — 绕过内核栈，接 [Ch6 mbuf](../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md) / burst API。

| 共用手段 | 说明 |
|----------|------|
| **大页** | 减 TLB/EPT 压力 — [Ch2](../chapter-02-cache-and-memory/notes/section-5-大页Hugepages.md) |
| **轮询** | 减 virtqueue notify / 中断 — [Ch7](../chapter-07-nic-performance-optimization/) |
| **SIMD** | 描述符/ mbuf 批量处理 — [Ch3](../chapter-03-parallel-computing/notes/section-4-数据并行与SIMD.md) |

**PMD：** `net_virtio` — 在 Guest 内仍走 Virtqueue，但 **数据面不进内核**。

---

### 三、后端变体（概念）

| 后端 | 说明 |
|------|------|
| **vhost-net** | 内核线程处理 virtqueue — 经典 KVM |
| **vhost-user** | 后端在 **用户态**（如 OVS、VPP）— NFV 常见 |
| **DPDK vhost** | DPDK 作 **后端** 服务多个 VM — 与 `net_virtio` 对称 |

本章聚焦 **前端优化**；后端轮询/零拷贝与 **OVS-DPDK** 等专题见官方 doc / 02-Advanced。

---

← [3. Virtqueue](./section-3-虚拟队列机制.md) · 下一节 [5. DPDK 深度优化](./section-5-DPDK深度优化.md)
