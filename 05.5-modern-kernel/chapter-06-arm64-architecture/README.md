# Ch6 ARM64 架构

> 来源: 笨叔《奔跑吧Linux内核》 + Bootlin
> 对标旧书: ULK3 (x86 only)

AArch64 特性、异常等级、系统寄存器。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 6.1 AArch64 特性 (笨叔) | `notes/01-aarch64-special.md` |
| 6.2 ARM64 架构讲义 (Bootlin) | `notes/02-arm64-architecture-bootlin.md` |

---

## HFT 关联

树莓派 5 (Cortex-A76) 的 AArch64 架构是 HFT 嵌入式平台的基础。理解 EL0-EL3 特权级和系统寄存器对调试至关重要。
