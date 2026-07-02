# 2. 硬件虚拟化基础

### Hypervisor 配置

| 类型 | 例子 | 结构 |
|------|------|------|
| **裸金属 / Type-1** | **Xen** | Hypervisor 直接在硬件上，管理 Guest |
| **托管型 / Type-2 模块** | **KVM** | Linux 内核 **kvm 模块** + QEMU 等 |

Guest 内仍是 **完整 Linux 内核** — 故 **Ch 6–14 的 BPF 工具可在 Guest 内直接运行**（与物理机类似，但指标含虚拟化开销）。

### AWS Nitro 等演进

| 特点 | 分析含义 |
|------|----------|
| 网络/存储 **卸载到 Nitro 卡** | 少传统 KVM 设备模拟路径 |
| Hypervisor 功能 **硬件化** | **专用 Xen/KVM 工具减少** |
| 性能分析 | 更多依赖 **通用** `runqlat`、`tcpretrans`、`biolatency`（Ch 6–10） |

**HFT 上云：** 先 **`cpustolen` / 云监控 CPU credit** — 再决定是否值得深挖 `kvmexits`。

---
