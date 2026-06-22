## 3.5 其他系统模型

| 模型 | 思路 | 与宏内核关系 |
|------|------|--------------|
| **PGO 内核** | 用 CPU 剖析数据 **配置文件引导编译**，热路径布局更优 | Linux 可选 profile-guided 构建 |
| **Unikernel** | 应用与内核 **编译为一体**，减抽象层、减 syscall | 专用场景；牺牲通用性换延迟 |
| **微内核 / 混合内核** | 最小内核 + 用户态服务；或 Mach + BSD 层（如早期 macOS） | Linux 为 **宏内核**；对比见架构课 a02 |

**HFT 现实：** 主流仍是 **Linux 宏内核 + 绑核/旁路（DPDK/XDP）**；Unikernel 多见于研究或极专用部署。

→ 架构对比：[episode-a02](../../../05-Linux-Kernel-Development/03_Course_Kernel_Architecture/episode-a02-宏内核与微内核.md)

---


---

← [本章导读](../README.md)
