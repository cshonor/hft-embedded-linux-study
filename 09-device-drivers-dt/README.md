# Linux 设备驱动 · 字符 / 平台驱动 · 设备树

**文件夹 09** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线07–10)

> **定位：** **内核态模块** — 补齐 HFT 链里「只写用户态」的缺口。  
> **范围：** 字符驱动 · platform 驱动 · **设备树（DTS/DTB）** · **非** MCU 裸机 HAL。  
> **前置：** [11 构建链](../08-embedded-boot-build/) · [05 内核](../05-linux-kernel/) · [01 C](../01-c-language/)  
> **书目原则：** **全外文** — LDD3 思想 + Madieu 实操；**不用** 国产驱动书。  
> **设备树不单开文件夹** — 与 platform/I2C/SPI 一起学（原 22 已删除）。

---

## 必读书（2 本）

| 序 | 书目 | 读什么 | 索引 |
|----|------|--------|------|
| **实操主书** | ***Linux Device Drivers Development*** — Madieu | 模块→字符→Platform→**DTS**→I2C/SPI/DMA（成书 4.x） | [OUTLINE](./modern-driver-practice/OUTLINE.md) · [评测](./MADIEU-EVAL.md) |
| **原理补课** | ***Linux Device Drivers*, 3rd** — LDD3 | 锁/并发/中断/DMA/LDM/PCI·USB（2.6；**无 DTS**） | [OUTLINE](./classic-driver-theory/OUTLINE.md) · [评测](./LDD3-EVAL.md) |

> **读序：** [Primer](../08-embedded-boot-build/primer-system-overview/) → **Madieu 动手** → 卡住锁/DMA/内存时 **回头 LDD3**。  
> **概念：** 设备树 ≠ UEFI — [DT FAQ](../08-embedded-boot-build/primer-system-overview/chapter-07-bootloaders/7.0-device-tree-vs-uefi.md)；加硬件 DTS/驱动 — [8.0](../08-embedded-boot-build/primer-system-overview/chapter-08-device-driver-basics/8.0-new-hw-dts-vs-driver.md)/[8.1](../08-embedded-boot-build/primer-system-overview/chapter-08-device-driver-basics/8.1-dts-driver-relationship.md)。  
> **读驱动源码：** 标准 C + 少量 GNU 扩展 — [GNU 速查](../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/DRIVER-GNU-C-CHEATSHEET.md)。  
> **禁止**把 LDD3 样例当 5.x 模板硬抄。四书分工：[FOUR-BOOKS-OVERLAP](../08-embedded-boot-build/FOUR-BOOKS-OVERLAP.md)。

```
09-device-drivers-dt/
├── README.md · LDD3-EVAL.md · MADIEU-EVAL.md
├── modern-driver-practice/   ← Madieu · 22 章
└── classic-driver-theory/           ← LDD3 · 18 章
```

---

## 与 HFT 链的关系

| HFT 已学 | 驱动段延伸 |
|----------|------------|
| 用户态 `epoll`/`mmap` | 内核 **poll/wait_queue** · **remap_pfn_range**（LDD3 Ch6/15） |
| 无锁 / spinlock 概念 | 内核 **spinlock_t** · 中断上下文（LDD3 Ch5） |
| [13 内核网络](../12-kernel-networking/) | 网卡驱动（LDD3 Ch17 / Madieu Ch22） |
| [13 DPDK](../13-dpdk/) | UIO/VFIO **旁路** vs 内核驱动 **标准路径** |

**HFT 退路：** 工业网关 / 飞控 **传感器 SPI/I2C/UART** — Madieu 主线。

---

## 核心技能清单

| 技能 | 主跟 |
|------|------|
| module_init/exit · 字符设备 | Madieu Ch2–4 · LDD3 Ch2–3 |
| platform + **设备树** | Madieu Ch5–6 · 官方 DT 文档（下表） |
| I2C / SPI / GPIO | Madieu Ch7–8、14 |
| 中断 / 锁 / DMA | Madieu Ch3、12 · LDD3 Ch5、10、15 |

---

## 设备树（官方文档 · 与 Madieu Ch6 并行）

| # | 文档 | 读什么 |
|---|------|--------|
| **1** | [Linux and the Devicetree（Usage Model）](https://docs.kernel.org/devicetree/usage-model.html) | `compatible` · platform 匹配 · FDT/DTB 启动链 |
| **2** | [Devicetree Spec — Usage](https://devicetree-specification.readthedocs.io/en/latest/usage-model.html) | DTS 语法 · `reg` / `interrupts` · phandle |
| **3** | [Bindings 索引](https://docs.kernel.org/devicetree/bindings/index.html) | 查外设 `compatible` |
| **选读** | [Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html) | DT overlay |

Madieu Ch6 大纲：[OUTLINE §Ch6](./modern-driver-practice/OUTLINE.md#ch6-the-concept-of-a-device-tree)

---

## 验收

- [ ] 写过一个 **最小字符驱动**（ioctl + read/write）  
- [ ] 能解释 **用户态 open() 如何落到驱动的 open**  
- [ ] 知道 **硬中断里不能 sleep**  
- [ ] 树莓派上改过 **DTS** 并匹配 platform/I2C 驱动  
- [ ] 读过 usage-model，能解释 DTB 从哪来、内核用来干什么  

**上一章：** [20 构建](../08-embedded-boot-build/) · **下一章：** [10 运动控制](../10-motion-control/)
