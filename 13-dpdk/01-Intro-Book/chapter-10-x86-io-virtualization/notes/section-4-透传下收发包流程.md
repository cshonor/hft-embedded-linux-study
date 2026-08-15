## 4. PCIe 网卡透传下的收发包流程

> 开启 **VT-x + VT-d** 后，客户机内 DPDK 流程与 **物理宿主机** 基本相同 — 核心差异：**多一层地址转换**

---

### 一、与裸金属对比

| 路径 | 地址转换 |
|------|----------|
| **裸金属 DPDK** | 设备/CPU 直接用 **HPA**（或大页物理地址） |
| **透传 VM 内 DPDK** | CPU 访存：**EPT**（GPA→HPA）；NIC DMA：**VT-d**（GPA→HPA） |

PMD **burst / 描述符环 / ol_flags** 逻辑不变 — [Ch6–7](../chapter-06-pcie-packet-io/) · [Ch7](../chapter-07-nic-performance-optimization/)

---

### 二、CPU 操作内存

- 客户机代码访问 **GPA**  
- **EPT** 硬件完成 GPA→HPA  
- 转换结果进 **CPU TLB / Cache**

→ 与 [Ch2 Cache/TLB](../chapter-02-cache-and-memory/) 调优仍相关（VM 内亦需 **绑核、NUMA**）

---

### 三、网卡 DMA 操作内存

- 设备发起 DMA 仍用 **GPA**（客户机视角）  
- **VT-d DMA 重映射** → HPA  
- 转换结果进 **IOTLB**

**强烈建议：大页（Hugepages）**

| 原因 | 说明 |
|------|------|
| **更少页表项** | EPT + I/O 页表 **映射条目↓** |
| **更高 IOTLB 命中** | DMA 地址 **更稳定、更粗粒度** |
| 与裸金属一致 | [Ch2 大页](../chapter-02-cache-and-memory/notes/section-5-大页Hugepages.md) · [Ch6 mempool](../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md) |

---

### 四、性能检查点

- [ ] VM **memory backing** 与 NIC **同 NUMA Node**（宿主机视角）  
- [ ] VF **直通** 而非 virtio-net  
- [ ] **1G/2M 大页** 在客户机 EAL 中启用  
- [ ] 对比 **同配置裸金属** baseline — 量化虚拟化税  

---

← [3. I/O 透传技术](./section-3-IO透传核心技术.md) · 下一节 [5. 配置与陷阱](./section-5-透传配置与常见问题.md)
