## 1. 本章定位

> **ULK Ch 13 I/O Architecture and Device Drivers** · 内核如何与 **真实硬件** 交互

---

### 一、本章讲什么

Ch 12 VFS 是 **通用文件抽象**；本章视线 **向下** 到总线、控制器与驱动：

| 主题 | 要点 |
|------|------|
| **硬件 I/O** | 端口 / MMIO、接口、控制器 |
| **驱动模型** | kobject、sysfs、device/driver/bus/class |
| **设备文件** | `/dev`、主次设备号、动态分配 |
| **I/O 完成** | 轮询 vs 中断、DMA |
| **字符设备** | `cdev`、`file_operations` 挂钩 |

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-IO体系结构.md) | 总线、I/O 端口、MMIO |
| [3](./section-3-设备驱动程序模型.md) | kobject、sysfs、四大组件 |
| [4](./section-4-设备文件.md) | major/minor、32 位扩展 |
| [5](./section-5-驱动通用特性.md) | polling、IRQ、DMA 映射 |
| [6](./section-6-字符设备驱动.md) | `cdev`、注册、VFS 挂钩 |

---

### 三、在 Linux 链上的位置

```
Ch 4  中断 / softirq / I/O 中断处理
Ch 12 VFS — open/read/write 到设备文件
Ch 13 I/O 与字符驱动（本章）
Ch 14 块设备驱动
Ch 16 文件访问 / 页缓存
```

HFT：网卡/NVMe 等多在用户态 **内核旁路**（DPDK 等）；本章仍有助于理解 **IRQ、DMA、/sys** 调参与传统设备路径。

---

← [Ch 13 导读](../README.md) · 下一节 [2. I/O 体系结构](./section-2-IO体系结构.md)
