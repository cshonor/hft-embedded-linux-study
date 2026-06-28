## 6. 小结与后续索引

---

### 一、本章主线

```
Multi-queue（并行管道）
    + RSS（均衡散列）
    + Flow Director（精确导流）
    + QoS / ptype offload（硬件语义）
    ↓
Run-to-Completion：一核一队、NUMA 就近
    ↓
RMT：Match+Action 统一理解智能网卡
```

**结论：** 深入挖掘 **硬件分流** → **卸载 CPU** → 为复杂应用腾算力。

---

### 二、后续章节索引

| Ch8 主题 | 继续读 |
|----------|--------|
| 硬件 offload | [chapter-09-hardware-offload](../chapter-09-hardware-offload/) 🔴 |
| PMD / burst | [chapter-03 PMD](../chapter-03-PMD与轮询模式.md) 🔴 |
| Cache / NUMA / per-core | [chapter-02-Cache与内存](../chapter-02-cache-and-memory/) 🔴 |
| 组播行情落地 | [chapter-05 组播](../chapter-05-组播行情接入.md) 🔴 |
| 内核 RSS 对照 | [13-LKN Ch14](../../../13-Linux-Kernel-Networking/chapter-14-advanced-topics/) |
| L3fwd 软件转发 | [Ch1 L3fwd](../chapter-01-dpdk-intro/notes/section-6-编程实例入门.md) |
| HFT 网络工程 | [15 ch06](../../../15-HFT-Low-Latency-Practice/) |
| XDP / 半旁路 | [02-Advanced note-XDP](../../02-Advanced-Book/notes/note-XDP与DPDK对照.md) |

---

← [5. RMT](./section-5-RMT抽象模型.md) · 下一章 [chapter-09 硬件 offload](../chapter-09-hardware-offload/) · [01-Intro README](../README.md)
