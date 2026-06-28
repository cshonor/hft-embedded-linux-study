## 6. 小结与后续索引

---

### 一、本章总结

**Virtio = 标准半虚拟化；DPDK = 在用户态把前端推到头：**

| 层次 | 要点 |
|------|------|
| **规范** | 前后端、特性协商、PCI/MMIO |
| **Virtqueue** | Descriptor + Available + Used |
| **权衡** | 易迁移、可过宿主机处理 vs 性能 < 透传 |
| **DPDK** | 固定 Available 映射、Indirect desc |

```
Ch10 透传 (VT-d / SR-IOV)
    ↓
Ch11 Virtio（本章）— 半虚拟化 · virtqueue · PMD 优化
    ↓
vhost / NFV 实践 · repo 零拷贝/组播
```

---

### 二、后续章节索引

| Ch11 主题 | 继续读 |
|----------|--------|
| I/O 透传对照 | [chapter-10-x86-io-virtualization](../chapter-10-x86-io-virtualization/) 🟡 |
| VF / SR-IOV | [chapter-08 §4](../chapter-08-flow-classification-multiqueue/notes/section-4-DPDK实战结合.md) 🔴 |
| mbuf / 大页 | [chapter-06-pcie-packet-io](../chapter-06-pcie-packet-io/) · [Ch2 大页](../chapter-02-cache-and-memory/) 🔴 |
| 轮询 / burst | [chapter-07-nic-performance-optimization](../chapter-07-nic-performance-optimization/) 🔴 |
| 零拷贝旁路 | [chapter-04-零拷贝与用户态旁路.md](../chapter-04-零拷贝与用户态旁路.md) 🔴 |
| XDP / 半旁路 | [02-Advanced note-XDP](../../02-Advanced-Book/notes/note-XDP与DPDK对照.md) |
| HFT 部署 | [15 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. DPDK 优化](./section-5-DPDK深度优化.md) · [Ch10 透传](../chapter-10-x86-io-virtualization/) · [01-Intro README](../README.md)
