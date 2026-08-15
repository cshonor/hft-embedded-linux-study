## 3. I/O 透传核心技术

---

### 一、透传的利弊

| 优点 | 代价 |
|------|------|
| **近裸机** I/O 性能 | 设备 **独占** → 硬件成本 ↑ |
| DPDK **直接控 NIC** | **动态迁移** 受限 |
| NFV 数据面标准路径 | 配置复杂（IOMMU、BIOS） |

---

### 二、Intel VT-d（DMA 重映射）

**问题：** VM 内设备 DMA 使用 **GPA** — 不经转换会 **写错宿主机内存**。

**VT-d：** 芯片组 **DMA 重映射硬件**

| 机制 | 作用 |
|------|------|
| **I/O 页表** | GPA → HPA **自动转换** |
| **IOTLB** | 缓存 DMA 地址转换 — 类比 CPU TLB |
| **Domain** | **隔离** 不同 VM/设备的 DMA 权限 |

→ 与 [Ch6 PCIe MRd/MWr](../chapter-06-pcie-packet-io/notes/section-2-PCIe事务与带宽.md) 同一 DMA 路径，多一层 remapping

---

### 三、PCIe SR-IOV

**单根 I/O 虚拟化** — 一卡多 **VF**：

```
PF (Physical Function) — 宿主机管理
    ├── VF0 → VM A
    ├── VF1 → VM B
    └── VFn …
```

| 特点 | 说明 |
|------|------|
| 每 **VF** 独立 | PCI 配置空间、队列、中断 |
| 解决 **独占** | 多 VM **共享** 同一物理网卡 |
| DPDK | 客户机绑定 **VF** 跑 PMD — [Ch8 VF](../chapter-08-flow-classification-multiqueue/notes/section-4-DPDK实战结合.md) |

**HFT：** 托管环境常申请 **SR-IOV VF + CPU 亲和**；无 VF 则退回 virtio，**延迟不可比**。

---

← [2. X86 虚拟化概述](./section-2-X86虚拟化概述.md) · 下一节 [4. 透传收发包流程](./section-4-透传下收发包流程.md)
