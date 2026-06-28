## 6. 小结与后续索引

---

### 一、本章总结

**虚拟化篇基石 — 透传让 DPDK in VM ≈ 裸金属：**

| 技术 | 作用 |
|------|------|
| **VT-x / VMCS** | CPU 非根模式 + 快速切换 |
| **EPT** | GPA→HPA，CPU 访存 |
| **VT-d / IOTLB** | DMA GPA→HPA，安全隔离 |
| **SR-IOV VF** | 一卡多 VM，避免独占 |
| **Hugepages** | 提高 EPT/IOTLB 命中 |

```
第一部分 Ch1–9（裸金属数据面）
    ↓
Ch10 I/O 虚拟化 — 虚拟化篇开篇 · VT-d · SR-IOV · 透传
    ↓
Ch11 Virtio — 半虚拟化 · virtqueue · DPDK 前端优化
    ↓
repo：零拷贝 · 组播 · 02-Advanced
```

---

### 二、后续章节索引

| Ch10 主题 | 继续读 |
|----------|--------|
| 半虚拟化 Virtio | [chapter-11-virtio-paravirtualization](../chapter-11-virtio-paravirtualization/) 🟡 |
| SR-IOV / VF 实战 | [chapter-08-flow-classification-multiqueue §4](../chapter-08-flow-classification-multiqueue/notes/section-4-DPDK实战结合.md) 🔴 |
| 大页 / NUMA | [chapter-02-cache-and-memory](../chapter-02-cache-and-memory/) 🔴 |
| PCIe / DMA | [chapter-06-pcie-packet-io](../chapter-06-pcie-packet-io/) 🔴 |
| PMD / VFIO | [chapter-03-PMD与轮询模式.md](../chapter-03-PMD与轮询模式.md) 🔴 |
| 硬件 offload | [chapter-09-hardware-offload](../chapter-09-hardware-offload/) 🔴 |
| 零拷贝旁路 | [chapter-04-零拷贝与用户态旁路.md](../chapter-04-零拷贝与用户态旁路.md) 🔴 |
| XDP / 半旁路 | [02-Advanced note-XDP](../../02-Advanced-Book/notes/note-XDP与DPDK对照.md) |
| HFT 部署 | [15 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. 配置与陷阱](./section-5-透传配置与常见问题.md) · [Ch9 offload](../chapter-09-hardware-offload/) · [01-Intro README](../README.md)
