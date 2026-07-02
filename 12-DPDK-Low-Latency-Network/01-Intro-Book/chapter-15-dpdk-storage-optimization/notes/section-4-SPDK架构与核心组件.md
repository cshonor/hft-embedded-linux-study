## 4. SPDK 架构与核心组件

> **SPDK（Storage Performance Development Kit）** — Intel 基于 **DPDK 框架** 的开源存储加速方案，面向 iSCSI 等以太网存储应用。

---

### 一、SPDK 在 DPDK 生态中的位置

```
DPDK（EAL / PMD / mempool / ring / 绑核）
    ↓ 扩展
SPDK（用户态 L4 + NVMe + 存储 Target 应用）
```

| 层级 | 负责方 |
|------|--------|
| L2/L3 报文 I/O | **DPDK** |
| L4 TCP/UDP | **SPDK 集成的用户态协议栈** |
| 块设备 I/O | **SPDK NVMe 驱动** |
| 存储协议 | **iSCSI Target 等示例应用** |

---

### 二、用户态 TCP/IP 栈

DPDK **不直接支持 TCP** — SPDK 需集成 L4 实现。本书重点提及：

| 栈 | 要点 |
|----|------|
| **Libuns**（书中命名） | **截获标准 Socket 操作** — 兼容现有应用；网络处理转入用户态；DPDK 队列 + 独立内存池 → **纯轮询、无锁流转** |
| **mTCP** | 高性能用户态 TCP 研究/生产实现 |
| **OpenFastPath（OFP）** | 另一用户态协议栈选项 |

**设计共性：** per-core 连接/缓冲、轮询 RX/TX、避免内核锁 — 与 [Ch7 poll 模式](../chapter-07-nic-performance-optimization/) 一致。

---

### 三、用户态 NVMe 驱动

| 特性 | 说明 |
|------|------|
| **轮询 completion** | 无中断风暴 — 延迟可预测 |
| **无内核锁** | 每队列独立 — 多核 **近线性扩展** |
| **线程↔队列绑定** | DPDK lcore 与 NVMe **QP** 亲和 |
| **性能** | 频繁读写下，**单核 IOPS** 远超内核 NVMe 驱动（书中测试结论） |

→ NVMe 命令集走 **PCIe** — 与 [Ch6](../chapter-06-pcie-packet-io/) 网卡 PMD 同属 **用户态 PCIe 驱动** 范式

---

### 四、高性能 iSCSI Target 实例

SPDK 提供 **iSCSI Target** 参考实现 — 端到端路径：

```
网卡 (DPDK PMD)
    → 用户态 TCP/IP 栈
    → iSCSI 协议解析（Target）
    → 用户态 NVMe 驱动
    → SSD 落盘
```

**全程：** 用户空间、轮询、无内核 block/TCP 路径。

---

### 五、与其他用户态栈选型（概念）

| 需求 | 倾向 |
|------|------|
| **最小改动现有 socket 应用** | Libuns 类 **socket 截获** 方案 |
| **全新存储服务** | 直接链接 SPDK API / mTCP |
| **多协议 NFV** | 可与 [Ch14 OVS](../chapter-14-ovs-dpdk-acceleration/) 分域 — OVS 管虚拟交换，SPDK 管存储面 |

---

← [3. 全栈优化思路](./section-3-全栈用户态优化思路.md) · 下一节 [5. 性能量化](./section-5-性能量化.md)
