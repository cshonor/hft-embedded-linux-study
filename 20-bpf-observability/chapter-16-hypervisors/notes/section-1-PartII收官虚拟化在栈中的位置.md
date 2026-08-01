# 1. Part II 收官：虚拟化在栈中的位置

```
裸金属 HFT（首选）
    ↓ 若上云/VM
Guest OS + 本章 Guest 工具（cpustolen、xenhyper…）
    ↓
Hypervisor / KVM（Host：kvmexits）
    ↓
物理 CPU / NIC / 盘
```

**与容器对比：**

| | Ch 15 容器 | Ch 16 VM |
|---|------------|----------|
| 隔离 | namespaces + cgroups | **硬件虚拟化** |
| 典型争用 | cgroup throttle | **CPU stolen、VM exit** |
| BPF 跑哪 |  mostly **Host** | **Guest 与 Host 均可** |

---
