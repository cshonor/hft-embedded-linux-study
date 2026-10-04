# Ch 10 X86 I/O 虚拟化 · X86 I/O Virtualization

> **01-Intro-Book** · 《深入浅出 DPDK》第十章 · **🟡 HFT 选读**  
> **第二部分「虚拟化技术篇」开篇** — VT-x/EPT、VT-d、SR-IOV、I/O 透传

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. X86 虚拟化概述 | [notes/section-2-X86虚拟化概述.md](./notes/section-2-X86虚拟化概述.md) |
| 3. I/O 透传核心技术 | [notes/section-3-IO透传核心技术.md](./notes/section-3-IO透传核心技术.md) |
| 4. 透传下收发包流程 | [notes/section-4-透传下收发包流程.md](./notes/section-4-透传下收发包流程.md) |
| 5. 透传配置与常见问题 | [notes/section-5-透传配置与常见问题.md](./notes/section-5-透传配置与常见问题.md) |
| 6. 小结与索引 | [notes/section-6-小结与索引.md](./notes/section-6-小结与索引.md) |

---

## HFT 视角：为什么"选读"而不是"跳过"

HFT 生产路径是**裸金属 + 独占网卡**，虚拟化看似与己无关——但三个现实场景会撞上本章：

| 场景 | 本章知识的用法 |
|------|----------------|
| 券商托管机房提供的是 **VM 网关**而非裸金属 | **SR-IOV VF + DPDK** 是 VM 里跑低延迟收发的**唯一可接受**路径；virtio 直接出局（见下一章） |
| 私有云/回测集群上部署策略 | VT-d **IOMMU** 的开销模型（IOTLB miss = 一次页表遍历）决定了为什么 VF 场景必须配**大页**——IOMMU 页表与 EPT 一样吃 TLB |
| 排查"云上延迟比物理机差一个量级" | 大概率不是算力问题，而是**没透传**：virtio 每次 kick/notify 都是一次 VM-exit（µs 级抖动，见 [Ch11](../chapter-11-virtio-paravirtualization/)） |

**一句话：** 交易主路径拒绝虚拟化；但**理解透传**能让你在"必须上云/必须 VM"的谈判里
准确说出要 SR-IOV 而不是 virtio，并知道该检查 IOMMU 大页。

---

## 相关

- 上一章：[chapter-09-hardware-offload/](../chapter-09-hardware-offload/)
- 下一章：[chapter-11-virtio-paravirtualization/](../chapter-11-virtio-paravirtualization/)
- 对照：[Ch8 VF/SR-IOV](../chapter-08-flow-classification-multiqueue/notes/section-4-DPDK实战结合.md) · [Ch2 大页](../chapter-02-cache-and-memory/notes/section-5-大页Hugepages.md) · [Ch6 PCIe/DMA](../chapter-06-pcie-packet-io/)
