# Linux 设备驱动 · 字符 / 平台驱动

**文件夹 21** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线19–24)

> **定位：** **内核态模块** — 补齐 HFT 链里「只写用户态」的缺口。  
> **范围：** 字符驱动 · platform 驱动 · **非** MCU 裸机 HAL。  
> **前置：** [20 构建链](../20-UBoot-Kernel-Build/) · [04–05 内核](../04-Linux-Kernel-Development/) · [02 C](../02-c-programming/)  
> **书目原则：** **全外文** — LDD3 思维 + Madieu 现代 API；**不用** 国产《Linux 内核驱动深度开发》。

---

## 必读书（2 本 · 按路线图顺序）

| 序 | 书目 | 读什么 |
|----|------|--------|
| **1** | ***Linux Device Drivers*, 3rd ed** — Corbet / Rubini / Kroah-Hartman | **LDD3** — file_operations · 并发 · platform 基础 · **驱动框架思想** |
| **2** | ***Linux Device Driver Development*, 2nd ed** — John Madieu | **5.x/6.x 现代驱动** — device model · 时钟/电源/总线 · **替代国产驱动书** |

> LDD3 内核版本偏老，但 **思维模型不变**；Madieu 补 **现代内核 API** 与工程实践。

---

## 与 HFT 链的关系

| HFT 已学 | 驱动段延伸 |
|----------|------------|
| 用户态 `epoll`/`mmap` | 内核 **poll/wait_queue** · **remap_pfn_range** |
| 无锁 / spinlock 概念 | 内核 **spinlock_t** · **中断上下文** 规则 |
| [13 内核网络](../13-Linux-Kernel-Networking/) | 网卡驱动是 **字符/网络设备** 的特例 |
| [14 DPDK](../14-DPDK-Low-Latency-Network/) | UIO/VFIO **旁路** vs 内核驱动 **标准路径** |
| [08 MikanOS](../08-system-low-level-hands-on/01-mikan-os/) | 自制 OS 无 Linux 驱动模型 — 本章是 **Linux 量产路径** |

**HFT 退路：** 工业网关 / 飞控 **传感器 SPI/I2C/UART** 驱动 — 同一套 LDD 技能。

---

## 核心技能清单

| 技能 | 说明 |
|------|------|
| **module_init/exit** | 可加载内核模块 |
| **字符设备** | cdev · read/write/ioctl |
| **platform driver** | 与 **设备树** 匹配（→ [22](../22-Device-Tree-Study/)） |
| **中断处理** | request_irq · 顶半部/底半部 |
| **并发** | 自旋锁 vs 信号量 · 不能睡眠的上下文 |

---

## 验收

- [ ] 写过一个 **最小字符驱动**（ioctl + read/write）  
- [ ] 能解释 **用户态 open() 如何落到驱动的 open**  
- [ ] 知道 **硬中断里不能 sleep**  

**上一章：** [20 构建](../20-UBoot-Kernel-Build/) · **下一章：** [22 设备树](../22-Device-Tree-Study/)
