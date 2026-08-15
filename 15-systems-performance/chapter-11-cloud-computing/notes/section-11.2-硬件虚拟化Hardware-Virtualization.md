## 11.2 硬件虚拟化（Hardware Virtualization）

### Hypervisor 与常见实现

| 类型 | 例子 | 特点 |
|------|------|------|
| **Type 1** | VMware ESXi、Hyper-V、Xen（部分） | 裸金属 Hypervisor |
| **Type 2 / 模块** | **KVM**（Linux 内核模块） | 宿主机也是 Linux |

每个 **VM** = 完整 Guest OS + 虚拟 vCPU/vNIC/vDisk。

### 性能开销来源

| 开销 | 说明 |
|------|------|
| **VM-EXIT / VM-ENTRY** | Guest ↔ Hypervisor 上下文切换 |
| **I/O 代理** | 虚拟磁盘/网卡经 Hypervisor 软件路径 — **延迟高** |
| **嵌套页表** | 内存虚拟化 — TLB 压力 |
| **Steal time** | Guest 内可见 — **vCPU 被宿主机抢走** 的时间 |

**Guest 内 `top`：** `%st`（steal）高 → **物理 CPU 争用**，非 Guest 算力足。

### 硬件直通与 Nitro

| 技术 | 效果 |
|------|------|
| **PCI Passthrough / SR-IOV** | 网卡/ NVMe **直通** Guest — 绕过软件 I/O 代理 |
| **AWS Nitro** | 网络/存储 offload 到专用硬件 — **接近裸机** 网络 |

**HFT：** 若必须在云上跑延迟敏感组件 — 选 **裸金属实例 / SR-IOV / 增强网络**，勿用普通虚拟网卡 + 共享 tenancy。

### 资源控制与观测

| 控制 | 机制 |
|------|------|
| vCPU 数量 | 限算力 |
| **Balloon driver** | 从 Guest **回收内存** 给宿主机 — Guest 内可用内存 **突然降** |
| 内存 overcommit | 宿主机卖超 — 触发 swap/压缩 |

| 观测位置 | 工具 |
|----------|------|
| **宿主机** | `kvm_stat`、`perf kvm`、`xentop`、Hypervisor CLI |
| **Guest 内** | 常规 perf/BPF — **见虚拟资源**，物理瓶颈可能 **不透明** |

**排障原则：** 延迟尖刺在 Guest 内无法解释 → **升 ticket 看宿主机** 或迁 **dedicated host**。

---


---

← [本章导读](../README.md)
