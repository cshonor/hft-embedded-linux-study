## 6. DDIO 与 NUMA

---

### 一、DDIO（Data Direct I/O）

**传统路径：**

```
网卡 → PCIe → DMA 写 → 主存(DRAM) → CPU cache miss → 载入 L3/L2/L1
```

**Intel DDIO：**

- 外部设备（网卡）可与 **CPU L3 Cache (LLC)** **直接交换数据**
- DMA 写 **直达 L3** — **绕过慢速 DRAM** 往返
- 降 **延迟** 与 **内存带宽** 压力

```
传统:  NIC → PCIe → DRAM → CPU load → L3 → L1     (~100ns+)
DDIO:  NIC → PCIe → L3 (直接写入) → L1             (~40ns)
```

**DDIO 技术要点：**

- Intel Ivy Bridge (2012+) 起支持
- BIOS 中需启用（通常默认开）
- 网卡 DMA 写入的目标地址如果在 LLC 中已有对应行（S 状态），则直接更新 LLC
- 如果不在 LLC 中，控制器会分配一个 LLC 行（WTT — Write Through to LLC）
- **更新量有限** — 最多 ~2 个 cache way（具体取决于实现），大块 DMA 仍会落 DRAM

**HFT 含义：** 共置 **Intel + 支持 DDIO 的网卡** 时，收包数据可能 **已在 LLC** — 与软件 prefetch 协同；无 DDIO 平台（AMD EPYC 部分型号、ARM）更依赖 **大页 + 预取 + 绑核**。

**验证 DDIO 是否生效：**

```bash
# 检查 CPU 是否支持 DDIO (Intel)
rdmsr 0x1A4  # DDIO 相关 MSR（如有输出则支持）

# 实测方法：对比开/关 DDIO 时的收包延迟
# perf stat -e cache-misses,cache-references ./my_dpdk_app
# 开启 DDIO 时 cache-misses 显著降低
```

 [Ch1 最佳实践](../../chapter-01-dpdk-intro/notes/section-3-性能最佳实践.md)

---

### 二、NUMA 架构

核数增加后 **SMP 总线** 瓶颈 → **NUMA**：

| 概念 | 说明 | 延迟 |
|------|------|------|
| **Node** | 每颗 CPU（或片）**直连** 本地内存 + 本地 PCIe | — |
| **本地访问** | CPU 访问本节点内存 | ~70-90 ns |
| **远程访问** | 跨 **QPI/UPI** 访问其他节点内存 | ~130-200 ns |
| **跨节点 PCIe** | CPU 访问其他节点的网卡 | +40-80 ns |

**双路服务器 NUMA 拓扑示例：**

```
        QPI/UPI (双向, ~9.6 GT/s)
    ┌──────────────────────────┐
    │                          │
┌───┴────────┐          ┌──────┴─────┐
│  Node 0    │          │  Node 1    │
│  CPU 0     │          │  CPU 1     │
│  cores 0-7 │          │  cores 8-15│
│  RAM 64GB  │          │  RAM 64GB  │
│  PCIe x16  │          │  PCIe x16  │
│  ┌──────┐  │          │  ┌──────┐  │
│  │NIC 0 │  │          │  │NIC 1 │  │
│  └──────┘  │          │  └──────┘  │
└────────────┘          └────────────┘
```

 [ULK Ch8 ZONE / 节点](../../../../16-linux-kernel-deep/chapter-08-memory-management/notes/section-2-页框管理.md)

---

### 三、DPDK NUMA 感知

**「本地设备本地处理」：**

| 资源 | 原则 | 检查方法 |
|------|------|----------|
| **网卡** | 插在 **Node N** 的 PCIe 槽 | `lspci -vvvs <bdf>` 看 NUMA node |
| **处理 lcore** | 绑定 **Node N** 的核 | `taskset` / `rte_lcore_id()` |
| **大页 / mempool** | 从 **Node N** 分配 | `--socket-mem=N,0` |
| **RX/TX queue** | 队列 **i** 由 **同 NUMA 核** 轮询 | `rte_eth_dev_socket_id()` |

**违反 NUMA 原则的代价：**

```c
/* ❌ 跨 NUMA — 网卡在 node0，mempool 在 node1 */
struct rte_mempool *pool = rte_pktmbuf_pool_create(
    "bad_pool", 32768, 256, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
    SOCKET_ID_ANY  /* 可能从任意节点分配！ */
);
/* 收包时 DMA 写入 node0 内存，但 mbuf 在 node1 → 每次 RX 都跨 NUMA 读 */

/* ✅ 正确 — 网卡和 mempool 同 NUMA */
int socket_id = rte_eth_dev_socket_id(port_id);
struct rte_mempool *pool = rte_pktmbuf_pool_create(
    "good_pool", 32768, 256, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
    socket_id  /* 与网卡同节点 */
);
```

**NUMA 拓扑检查：**

```bash
# 查看 NUMA 拓扑和设备分布
lstopo-no-graphics --no-io  # 简化拓扑
numactl --hardware           # NUMA 节点和 CPU 分布
cat /sys/bus/pci/devices/0000:81:00.0/numa_node  # 网卡所在 NUMA 节点

# DPDK 查看
dpdk-devbind.py --status  # 网卡 BDF 和驱动
```

违反 → **远程内存 + 远程 PCIe** — tail latency **恶化**，HFT 场景下可能出现 **偶发 200ns+ 尖刺**。

**工具：** `lstopo` · `dpdk-devbind.py` · EAL `--socket-mem` / `-l` 绑核。

---

### 四、本章小结

```
Cache 层次 + 软件预取 → 隐藏延迟
对齐 + per-core → 避免 MESI 风暴
大页 → TLB 命中
DDIO → 数据进 LLC
NUMA → 本地内存本地 NIC
    ↓
mbuf/mempool 在正确内存上预分配
```

---

### 五、后续章节索引

| Ch2 主题 | 继续读 |
|----------|--------|
| mbuf / mempool | [Ch6 §6 mbuf与Mempool](../../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md) 🔴 |
| 并行 / SIMD | [chapter-03-并行计算](../../chapter-03-parallel-computing/) 🔴 |
| PMD 收发包 | [chapter-07 网卡性能优化](../../chapter-07-nic-performance-optimization/) 🔴 |
| 零拷贝旁路 | [chapter-04 同步互斥](../../chapter-04-synchronization/) 🔴 |
| CSAPP / Hennessy | [01 Ch6](../../../../02-computer-systems/chapter-06-memory-hierarchy/) · [02 Ch2](../../../../15-computer-architecture/chapter-02-memory-hierarchy-design/) |
| ULK 内存 | [06 Ch8/9/17](../../../../16-linux-kernel-deep/chapter-08-memory-management/) |
| HFT 工程 | [14 ch05/ch07](../../../../14-hft-engineering/chapter-05-os-kernel-tuning/) |

---

← [5. 大页](./section-5-大页Hugepages.md) · 下一章 [Ch3 并行计算](../../chapter-03-parallel-computing/)
