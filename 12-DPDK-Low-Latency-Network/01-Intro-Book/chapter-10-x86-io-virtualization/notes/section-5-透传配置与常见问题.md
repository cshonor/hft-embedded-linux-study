## 5. I/O 透传配置的常见问题

---

### 一、BIOS / 固件

| 项 | 要求 |
|----|------|
| **VT-x** | 必须 **开启** — CPU 虚拟化 |
| **VT-d** | 必须 **开启** — DMA 重映射（名称可能为 Intel Virtualization Technology for Directed I/O） |

未开 VT-d → 透传 **无法安全** 工作或性能 **严重受限**。

---

### 二、Linux 内核参数

激活 **IOMMU**（VT-d 在 Linux 侧接口）：

```text
intel_iommu=on
```

（AMD 平台为 `amd_iommu=on`）

| 现象 | 常见原因 |
|------|----------|
| VM 启动报 **找不到 IOMMU** | 未加 `intel_iommu=on` 或 BIOS 未开 VT-d |
| VFIO 绑定失败 | IOMMU 组、权限、内核驱动占用 |

→ DPDK 常用 **VFIO** 替代旧 UIO — [Ch7 混合中断 UIO/VFIO](../chapter-07-nic-performance-optimization/notes/section-2-轮询与混合中断模式.md)

---

### 三、部署检查清单

- [ ] `dmesg | grep -i iommu` 确认激活  
- [ ] `lspci` 见 PF/VF；`virsh nodedev-list` / 云厂商 **VF 配额**  
- [ ] 宿主机 **不把 PF 驱动** 与 VF 争用（常见：PF 绑 igb/i40e，VF 直通 VM）  
- [ ] 客户机内 **Hugepages + isolcpus** 与裸金属同规范 [Ch7](../chapter-07-nic-performance-optimization/notes/section-4-平台优化与配置调优.md)  
- [ ] 确认 **live migration** 需求 — 透传 VF **通常不可** 热迁  

---

### 四、与 NFV 的关系

**I/O 透传 + SR-IOV** = 虚拟机内 **极致 I/O** 的硬件基础 → **NFV 数据面** 跑 DPDK 的行业默认组合。

控制面（Orchestrator、OVS 慢路径）仍可 virtio — **数据面 VF 直通**。

---

← [4. 透传收发包流程](./section-4-透传下收发包流程.md) · 下一节 [6. 小结与索引](./section-6-小结与索引.md)
