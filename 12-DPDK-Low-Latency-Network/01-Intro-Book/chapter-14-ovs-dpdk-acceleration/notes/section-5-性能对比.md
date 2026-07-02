## 5. DPDK 加速 OVS 的性能对比

> 本章引用 **Intel 测试数据**（256 字节小包）量化 DPDK 改造收益 — 具体数值随硬件/版本变化，**量级与相对倍数** 具有参考价值。

---

### 一、测试前提

| 项 | 说明 |
|----|------|
| **报文大小** | 256 字节 |
| **对比基线** | 传统内核快路径 OVS |
| **加速版** | OVS + DPDK（dpif-netdev / netdev-dpdk） |

→ 小包场景对 **PPS（包每秒）** 压力最大 — 最能暴露内核与用户态切换开销。

---

### 二、PHY-PHY（物理网口 ↔ 物理网口）

```
NIC ──PMD──► OVS (用户态) ──PMD──► NIC
```

| 指标 | 结果 |
|------|------|
| **相对传统 OVS** | **11.4×** 吞吐提升 |
| **原因** | 全程用户态 PMD — 无内核转发、无 netdevice 栈 |

→ 与 [Ch7 burst / 向量 PMD](../chapter-07-nic-performance-optimization/) 优化直接对应

---

### 三、PHY-VM-PHY（物理网口 ↔ VM ↔ 物理网口）

```
NIC ──► OVS ──vhost──► VM (Virtio PMD) ──► OVS ──► NIC
```

| 配置 | 相对传统 OVS |
|------|--------------|
| **单核** 处理 OVS 转发 | **7.1×** |
| **双核** 处理 OVS 转发 | **12.9×** |

| 观察 | 含义 |
|------|------|
| 路径更复杂（含 VM 往返） | 绝对倍数低于 PHY-PHY，但仍 **数量级提升** |
| **双核 > 单核** 且超过 PHY-PHY 单核倍数 | **多核并行扩展性** 良好 — 与 [Ch3 并行](../chapter-03-parallel-computing/) 设计一致 |

---

### 四、闭环调优提示（衔接 Ch13）

性能倍数 **不是开箱即用** — 需配合：

| 维度 | 手段 |
|------|------|
| **硬件** | 多核、10G/25G NIC、NUMA 对齐 |
| **OS** | 大页、hugepages mount、isolcpus |
| **OVS** | PMD 线程数、RX/TX 队列、流表 cache |
| **VM** | Virtio 多队列、vhost-user 后端绑核 |

→ [Ch13 闭环性能分析](../chapter-13-dpdk-nfv/notes/section-4-VNF评估与性能分析.md)

---

← [4. netdev-dpdk 接口](./section-4-netdev-dpdk接口类型.md) · 下一节 [6. 小结](./section-6-小结与索引.md)
