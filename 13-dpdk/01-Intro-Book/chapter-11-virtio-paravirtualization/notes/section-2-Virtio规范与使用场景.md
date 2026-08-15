## 2. Virtio 规范与使用场景

---

### 一、技术定位

**Virtio** = 半虚拟化 **设备抽象接口规范**

| 角色 | 位置 | 示例 |
|------|------|------|
| **前端驱动** | 客户机（Guest） | Linux `virtio_net`、DPDK `net_virtio` |
| **后端驱动** | 宿主机（Host） | **vhost-net**、QEMU 设备模型 |

**生态：** Qemu/KVM 标配；Windows/Linux Guest 广泛支持。

---

### 二、与 I/O 透传的权衡

| 选 Virtio 当 | 选透传当 |
|--------------|----------|
| 需 **宿主机处理**（防火墙、LB、OVS） | 数据面 **极致性能** |
| 需 **虚拟机动态迁移** | **SR-IOV VF** 可分配 |
| **标准化** 运维、多租户 | tick 延迟 **敏感** |

→ [Ch10 三种 I/O 模型](../chapter-10-x86-io-virtualization/notes/section-2-X86虚拟化概述.md)

---

### 三、配置与特性协商

**传输接口：** PCI、MMIO、Channel I/O 等（VM 内常见 **Virtio-PCI**）。

**初始化流程（概念）：**

```
1. 前端读后端 **特性位 (feature bits)**
2. 双方协商 — 仅启用 **交集** 能力
3. 配置 **中断**（如 MSI-X）或 **轮询模式**
4. 建立 **Virtqueue(s)**
```

**配置空间：** **传统模式 (Legacy)** vs **现代模式 (Modern)** — 寄存器布局不同，DPDK/PMD 需匹配。

**常见特性（后文相关）：**

- `VIRTIO_F_RING_INDIRECT_DESC` — 间接描述符（§5）  
- `VIRTIO_F_MRG_RXBUF`、多队列等 — 视版本与后端  

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. Virtqueue](./section-3-虚拟队列机制.md)
