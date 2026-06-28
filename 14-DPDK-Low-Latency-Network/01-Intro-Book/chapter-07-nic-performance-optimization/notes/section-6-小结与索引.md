## 6. 小结与后续索引

---

### 一、本章总结

**网卡性能 = 模式 + 微优化 + 平台 + 参数：**

| 层次 | 要点 |
|------|------|
| **轮询** | 默认 **纯 poll**；空闲省 CPU 用 **混合中断**（牺牲首包） |
| **Burst / SIMD** | 凑 Cache Line、藏内存延迟、减 CL 冲突 |
| **平台** | Extended Tag、**同 NUMA**、`isolcpus` |
| **测试** | 多 **随机流** 防 RSS 假瓶颈 |
| **RX 环深** | 128 默认；高速/易丢包 → **512–1024** |

```
Ch6 PCIe / DMA / mbuf
    ↓
Ch7 网卡性能优化（本章）— poll、burst、平台、环深
    ↓
Ch8 流分类与多队列 — RSS / FD 硬件分流
    ↓
PMD stub / 组播落地
```

---

### 二、后续章节索引

| Ch7 主题 | 继续读 |
|----------|--------|
| RSS / 多队列 | [chapter-08-flow-classification-multiqueue](../chapter-08-flow-classification-multiqueue/) 🔴 |
| PMD / 官方 API | [chapter-03-PMD与轮询模式.md](../chapter-03-PMD与轮询模式.md) 🔴 |
| PCIe / MMIO | [chapter-06-pcie-packet-io](../chapter-06-pcie-packet-io/) 🔴 |
| NUMA / 大页 | [chapter-02-cache-and-memory](../chapter-02-cache-and-memory/) 🔴 |
| ILP / SIMD | [chapter-03-parallel-computing](../chapter-03-parallel-computing/) 🔴 |
| 零拷贝旁路 | [chapter-04-零拷贝与用户态旁路.md](../chapter-04-零拷贝与用户态旁路.md) 🔴 |
| HFT 网络 | [15 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. 队列长度](./section-5-队列长度及阈值设置.md) · 下一章 [chapter-08 流分类](../chapter-08-flow-classification-multiqueue/) · [Ch6 PCIe](../chapter-06-pcie-packet-io/)
