# Linux 设备驱动 · 字符 / 平台驱动

**文件夹 21** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线19–24)

> **定位：** **内核态模块** — 补齐 HFT 链里「只写用户态」的缺口。  
> **范围：** 字符驱动 · platform 驱动 · **非** MCU 裸机 HAL。  
> **前置：** [20 构建链](../20-UBoot-Kernel-Build/) · [04–05 内核](../04-Linux-Kernel-Development/) · [02 C](../02-c-programming/)  
> **书目原则：** **全外文** — LDD3 思想 + Madieu 实操；**不用** 国产驱动书。

---

## 必读书（2 本）

| 序 | 书目 | 读什么 | 索引 |
|----|------|--------|------|
| **实操主书** | ***Linux Device Drivers Development*** — Madieu | 模块→字符→Platform→**DTS**→I2C/SPI/DMA（成书 4.x） | [OUTLINE](./linux-device-drivers-development/OUTLINE.md) · [评测](./MADIEU-EVAL.md) |
| **原理补课** | ***Linux Device Drivers*, 3rd** — LDD3 | 锁/并发/中断/DMA/LDM/PCI·USB（2.6；**无 DTS**） | [OUTLINE](./linux-device-drivers-3rd/OUTLINE.md) · [评测](./LDD3-EVAL.md) |

> **读序：** [Primer](../20-UBoot-Kernel-Build/embedded-linux-primer/) → **Madieu 动手** → 卡住锁/DMA/内存时 **回头 LDD3**。  
> **禁止**把 LDD3 样例当 5.x 模板硬抄。四书分工：[FOUR-BOOKS-OVERLAP](../20-UBoot-Kernel-Build/FOUR-BOOKS-OVERLAP.md)。

```
21-Linux-Device-Driver/
├── README.md · MADIEU-EVAL.md · LDD3-EVAL.md
├── linux-device-drivers-development/   ← Madieu · 22 章
└── linux-device-drivers-3rd/           ← LDD3 · 18 章
```

---

## 与 HFT 链的关系

| HFT 已学 | 驱动段延伸 |
|----------|------------|
| 用户态 `epoll`/`mmap` | 内核 **poll/wait_queue** · **remap_pfn_range**（LDD3 Ch6/15） |
| 无锁 / spinlock 概念 | 内核 **spinlock_t** · 中断上下文（LDD3 Ch5） |
| [13 内核网络](../13-Linux-Kernel-Networking/) | 网卡驱动（LDD3 Ch17 / Madieu Ch22） |
| [14 DPDK](../14-DPDK-Low-Latency-Network/) | UIO/VFIO **旁路** vs 内核驱动 **标准路径** |

**HFT 退路：** 工业网关 / 飞控 **传感器 SPI/I2C/UART** — Madieu 主线。

---

## 核心技能清单

| 技能 | 主跟 |
|------|------|
| module_init/exit · 字符设备 | Madieu Ch2–4 · LDD3 Ch2–3 |
| platform + **设备树** | Madieu Ch5–6 → [22](../22-Device-Tree-Study/) |
| I2C / SPI / GPIO | Madieu Ch7–8、14 |
| 中断 / 锁 / DMA | Madieu Ch3、12 · LDD3 Ch5、10、15 |

---

## 验收

- [ ] 写过一个 **最小字符驱动**（ioctl + read/write）  
- [ ] 能解释 **用户态 open() 如何落到驱动的 open**  
- [ ] 知道 **硬中断里不能 sleep**  
- [ ] 树莓派上改过 **DTS** 并匹配 platform/I2C 驱动  

**上一章：** [20 构建](../20-UBoot-Kernel-Build/) · **下一章：** [22 设备树](../22-Device-Tree-Study/)
