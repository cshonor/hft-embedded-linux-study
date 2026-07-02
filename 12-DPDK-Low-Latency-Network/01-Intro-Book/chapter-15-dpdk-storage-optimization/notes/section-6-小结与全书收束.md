## 6. 小结与全书收束

---

### 一、本章总结

**DPDK 理念不限于 CT（通信）— 同样适用于 IT（存储）。**

| 传统内核存储路径 | SPDK 用户态路径 |
|------------------|-----------------|
| 多次拷贝（网卡↔内核↔用户↔驱动） | 网卡 → 用户态 TCP → NVMe **尽量零拷贝** |
| syscall + 上下文切换 | **纯轮询** 稳态 |
| 内核 TCP/block 锁 | **per-core 无锁队列** |

**SPDK 三板斧：**

```
用户态 TCP/IP（Libuns / mTCP / OFP）
    +
用户态 NVMe 驱动（轮询 QP）
    +
iSCSI Target 等应用（端到端示范）
    ↓
4KB 随机读 iSCSI：~7× vs LIO；多核近线性 IOPS
```

---

### 二、《深入浅出 DPDK》全书脉络收束

```
Part 1  Ch1–9   裸金属数据面
        CPU/Cache/并行/同步 → 转发/PCIe/NIC/offload
              ↓
Part 2  Ch10–12 虚拟化 I/O
        透传 → Virtio → vhost-user
              ↓
Part 3  Ch13–15 应用篇
        Ch13 NFV/VNF 方法论
        Ch14 OVS-DPDK 虚拟交换
        Ch15 SPDK 网络存储 ← 压轴
```

| DPDK 核心机制 | 全书出现 |
|---------------|----------|
| **旁路内核 / PMD** | Ch1、7、14、15 |
| **大页 / mempool / mbuf** | Ch2、6 |
| **多核 / SIMD** | Ch3 |
| **无锁 ring / 同步** | Ch4 |
| **Pipeline / Match+Action** | Ch5、13 |
| **PCIe / DMA** | Ch6、15（NVMe） |
| **硬件 offload** | Ch9 |
| **虚拟化接口** | Ch10–12、13–14 |

---

### 三、读完 Intro 之后

| 方向 | 路径 |
|------|------|
| **进阶网络全家桶** | [02-Advanced-Book](../../02-Advanced-Book/) — RDMA、XDP、选型 |
| **HFT 落地** | [15-HFT-Low-Latency-Practice](../../../15-HFT-Low-Latency-Practice/) |
| **repo 主题 stub** | [mbuf](../notes/chapter-02-mbuf与内存池.md) · [PMD](../notes/chapter-03-PMD与轮询模式.md) · [零拷贝](../notes/chapter-04-零拷贝与用户态旁路.md) · [组播](../notes/chapter-05-组播行情接入.md) |
| **性能方法论** | [03 SysPerf](../../../13-Systems-Performance-2nd/) |
| **递进说明** | [note-DPDK实体书递进](../notes/note-DPDK实体书递进.md) |

**结语：** 云存储与 NFV 同属 **通用 X86 + 用户态 I/O** 范式 — DPDK/SPDK 提供 **可复用的性能工程语言**（绑核、轮询、预分配、无锁），而非单一产品。

---

### 四、本章索引

| Ch15 主题 | 继续读 |
|----------|--------|
| OVS / NFV | [chapter-14](../chapter-14-ovs-dpdk-acceleration/) · [chapter-13](../chapter-13-dpdk-nfv/) 🟡 |
| PCIe / NVMe | [chapter-06-pcie-packet-io](../chapter-06-pcie-packet-io/) 🔴 |
| 无锁 / mempool | [chapter-04](../chapter-04-synchronization/) · [chapter-02](../chapter-02-cache-and-memory/) 🔴 |
| 进阶 / RDMA | [02-Advanced-Book](../../02-Advanced-Book/) 🟡 |

---

← [5. 性能量化](./section-5-性能量化.md) · [Ch14 OVS](../chapter-14-ovs-dpdk-acceleration/) · [01-Intro README](../README.md)
