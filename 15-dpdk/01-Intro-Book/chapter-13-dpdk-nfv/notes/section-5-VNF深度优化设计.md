## 5. VNF 深度优化设计

---

### 一、虚拟网络接口选型

NFVI 提供给 VNF 的 **四种** 典型接口：

| 接口 | 机制 | 性能 | 运维 |
|------|------|------|------|
| **IVSHMEM** | Qemu **VM 间共享内存** | 高（同 host VM2VM） | 特殊拓扑 |
| **Virtio** | 半虚拟化 | 中 | **热迁移**、标准 |
| **SR-IOV VF** | 透传虚拟功能 | **近裸金属** | 迁移差 |
| **PF 透传** | 整卡直通 | 最高 | 独占 |

**权衡：**

- 追求 **线速 / 低抖动** → **VF / PF** [Ch10](../chapter-10-x86-io-virtualization/)  
- 追求 **弹性、多租户** → **Virtio + vhost** [Ch11–12](../chapter-12-vhost-optimization/)  

→ [Ch8 VF 实战](../chapter-08-flow-classification-multiqueue/notes/section-4-DPDK实战结合.md)

---

### 二、QoS：多 VNF 共享 X86

多 VNF 共平台 → 争用：

- **最后一级 Cache（LLC）**  
- **内存带宽**  
- **核、PCIe**

**Intel 平台辅助（DPDK 可结合）：**

| 技术 | 作用 |
|------|------|
| **CMT** (Cache Monitoring) | 监控 LLC 占用 |
| **MBM** (Memory Bandwidth Monitoring) | 监控内存带宽 |
| **CAT** (Cache Allocation Technology) | **划分 LLC 给不同 VNF** |

**效果：** 降低 **应用间干扰** 与 **时延抖动** — 共置 HFT 与 noisy neighbor 问题同族。

→ [Ch2 伪共享 / per-core](../chapter-02-cache-and-memory/notes/section-4-Cache一致性与无锁设计.md)

---

### 三、设计检查清单

- [ ] VNF 分型与 **接口** 匹配场景  
- [ ] 数据面 **Pipeline + ring** 减跨核共享 [Ch5](../chapter-05-packet-forwarding/)  
- [ ] **CAT/绑核** 隔离关键 VNF  
- [ ] Crypto / DPI 走 **CryptoDev / Hyperscan** 而非纯 CPU 扫包  

---

← [4. VNF 评估](./section-4-VNF评估与性能分析.md) · 下一节 [6. 实例与小结](./section-6-实例与小结.md)
