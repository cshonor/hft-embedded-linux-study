## 11.4 轻量级硬件虚拟化（MicroVM）

### Firecracker 等

| 特点 | 说明 |
|------|------|
| **精简 Hypervisor** | 代码面小 — AWS Lambda/Fargate 等底层 |
| **无传统 BIOS/PCI 模拟** | 启动快、内存 **< 5MB** 级开销 |
| **隔离性** | 强于容器（独立 Guest 内核） |
| **密度** | 高于传统 VM |

**观测：** 与 KVM 类似 — **Guest 内有完整内核**，可用 **perf/BPF**；宿主机仍用 Hypervisor 工具。

**HFT：** 研究/沙箱隔离可用；**共置 tick 引擎** 仍优先 **裸金属** 而非 MicroVM。

---


---

← [本章导读](../README.md)
