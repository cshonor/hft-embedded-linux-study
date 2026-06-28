## 6. 小结与索引

---

### 一、本章总结

**虚拟交换机性能 = NFV 基础设施效率的关键一环。**

| 传统 OVS | DPDK OVS |
|----------|----------|
| 内核 `openvswitch.ko` 快路径 | **`dpif-netdev` 用户态快路径** |
| 内核 netdevice I/O | **`netdev-dpdk` + PMD / vhost** |
| NFV 高负载下瓶颈明显 | Intel 测试：**7–13×** 量级提升 |

**融入的 DPDK 核心机制：**

```
旁路内核 → PMD 轮询 → 大页 / 绑核 → 无锁队列 → 多核扩展
    ↑                                              ↑
 Ch1–7 裸金属基础                          Ch3/Ch4 并行与 ring
```

**与 Ch13 关系：** NFVI 层 **OVS-DPDK** 是 VNF 互联的 **默认开源选项** 之一 — 与 OpenStack、OPNFV 生态深度绑定。

---

### 二、全书脉络（应用篇）

```
Ch13 NFV 架构 / VNF 方法论
    ↓
Ch14 OVS-DPDK（虚拟交换机数据面）
    ↓
Ch15 SPDK 网络存储（应用篇压轴）← 见 [chapter-15](../chapter-15-dpdk-storage-optimization/)
    ↓
02-Advanced / HFT 落地
```

---

### 三、后续索引

| Ch14 主题 | 继续读 |
|----------|--------|
| NFV / NFVI | [chapter-13-dpdk-nfv](../chapter-13-dpdk-nfv/) 🟡 |
| vhost-user | [chapter-12-vhost-optimization](../chapter-12-vhost-optimization/) 🟡 |
| Virtio 前端 | [chapter-11-virtio-paravirtualization](../chapter-11-virtio-paravirtualization/) 🟡 |
| Match+Action / Pipeline | [chapter-05-packet-forwarding](../chapter-05-packet-forwarding/) 🔴 |
| 进阶网络（XDP 等） | [02-Advanced-Book](../../02-Advanced-Book/) 🟡 |
| HFT | [15 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. 性能对比](./section-5-性能对比.md) · [Ch13 NFV](../chapter-13-dpdk-nfv/) · [01-Intro README](../README.md)
