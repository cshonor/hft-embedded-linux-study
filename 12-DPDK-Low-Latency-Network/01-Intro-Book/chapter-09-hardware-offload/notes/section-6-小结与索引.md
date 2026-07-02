## 6. 小结与后续索引

---

### 一、本章总结

**硬件 offload = 网卡能力增强 + 软件正确协同：**

| 类 | 功能 | CPU 释放点 |
|----|------|------------|
| **计算/更新** | VLAN、Checksum、PTP、Tunnel | 剥 Tag、扫包校验、打戳 |
| **分片 TSO** | 大块 TCP → 多 MTU 段 | 发送路径切分与 checksum |
| **组包 RSC** | 多 TCP 段 → 大 PDU | 接收解析与排序 |
| **DPDK** | `ol_flags`、`l2/l3_len`、TX 上下文 | 声明式驱动硬件 |

```
Ch7 软件调优 → Ch8 硬件分流 → Ch9 offload → Ch10 I/O 虚拟化（虚拟化篇）
```

---

### 二、后续章节索引

| Ch9 主题 | 继续读 |
|----------|--------|
| I/O 虚拟化 / SR-IOV | [chapter-10-x86-io-virtualization](../chapter-10-x86-io-virtualization/) 🟡 |
| mbuf 字段 | [chapter-06-pcie-packet-io §6](../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md) 🔴 |
| RSS / ptype | [chapter-08-flow-classification-multiqueue](../chapter-08-flow-classification-multiqueue/) 🔴 |
| PMD / dev配置 | [chapter-03-PMD与轮询模式.md](../chapter-03-PMD与轮询模式.md) 🔴 |
| 零拷贝旁路 | [chapter-04-零拷贝与用户态旁路.md](../chapter-04-零拷贝与用户态旁路.md) 🔴 |
| 组播行情 | [chapter-05-组播行情接入.md](../chapter-05-组播行情接入.md) 🔴 |
| 内核 offload | [13-LKN](../../../11-Linux-Kernel-Networking/) |
| HFT 网络 | [15 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. DPDK 协同](./section-5-DPDK软件接口与协同.md) · 下一章 [chapter-10 I/O 虚拟化](../chapter-10-x86-io-virtualization/) · [Ch8 流分类](../chapter-08-flow-classification-multiqueue/)
