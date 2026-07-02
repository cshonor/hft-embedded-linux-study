# Linux 设备驱动 · 字符 / 平台驱动

**文件夹 20** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线18–23)

> **定位：** **内核态模块** — 补齐 HFT 链里「只写用户态」的缺口。  
> **范围：** 字符驱动 · platform 驱动 · **非** MCU 裸机 HAL。  
> **前置：** [19 构建链](../19-UBoot-Kernel-Build/) · [05–06 内核](../03-Linux-Kernel-Development/) · GNU-C（《嵌入式 C 语言自我修养》）

---

## 必读书（2 本 · 经典 + 现代）

| # | 书目 | 读什么 |
|---|------|--------|
| 1 | **《Linux 设备驱动程序》LDD3** | **字符驱动** · file_operations · 并发 · platform 基础 |
| 2 | **《Linux 内核驱动深度开发》** | **新版内核** API · device model · 现代总线/时钟/电源 |

> LDD3 偏老但 **思维模型** 不变；第二本补 **5.x/6.x** 接口差异。

---

## 与 HFT 链的关系

| HFT 已学 | 驱动段延伸 |
|----------|------------|
| 用户态 `epoll`/`mmap` | 内核 **poll/wait_queue** · **remap_pfn_range** |
| 无锁 / spinlock 概念 | 内核 **spinlock_t** · **中断上下文** 规则 |
| [13 内核网络](../11-Linux-Kernel-Networking/) | 网卡驱动是 **字符/网络设备** 的特例 |
| [14 DPDK](../12-DPDK-Low-Latency-Network/) | UIO/VFIO **旁路** vs 内核驱动 **标准路径** |

**HFT 退路：** 工业网关 / 飞控 **传感器 SPI/I2C/UART** 驱动 — 同一套 LDD 技能。

---

## 核心技能清单

| 技能 | 说明 |
|------|------|
| **module_init/exit** | 可加载内核模块 |
| **字符设备** | register_chrdev / cdev · read/write/ioctl |
| **platform driver** | 与 **设备树** 匹配（→ [21](../21-Device-Tree-Study/)） |
| **中断处理** | request_irq · 顶半部/底半部 |
| **并发** | 自旋锁 vs 信号量 · 不能睡眠的上下文 |

---

## 验收

- [ ] 写过一个 **最小字符驱动**（ioctl + read/write）  
- [ ] 能解释 **用户态 open() 如何落到驱动的 open**  
- [ ] 知道 **硬中断里不能 sleep**  

**上一章：** [19 构建](../19-UBoot-Kernel-Build/) · **下一章：** [21 设备树](../21-Device-Tree-Study/)
