# 01 · 定位：嵌入式 Linux 到底是什么

> **本节讲什么：** 在动手之前先钉死几个容易混淆的概念——嵌入式 Linux 不是"小一号的桌面 Linux"，RTOS 不是"更快的 Linux"，SoC 不是"CPU + 主板"。
> **为什么要先定这个：** 后面所有步骤（编内核、做 rootfs、写驱动）的选择，都取决于这块板子属于哪一类系统。定位错了，后面全是白工。
> **对应动手：** [P5 Phase A](../../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（把板子当 Linux 机器熟悉起来）

---

## 要点

| 概念 | 结论 |
|------|------|
| **Linux vs RTOS** | Linux 有 MMU、有用户/内核隔离、延迟是**统计意义**上的；RTOS 延迟是**确定性**的。要硬实时得打 PREEMPT_RT，且仍不是 RTOS |
| **SoC vs CPU** | BCM2712 是 **SoC**：CPU 核 + 外设控制器 + 内存控制器全在一颗芯片上。没有"南北桥"，没有 PCIe 枚举那一套 PC 叙事 |
| **BSP 是什么** | 板级支持包只是**模板**，不是产品。拿到自研 PCB 时，BSP 能给的是"能起来"，不是"能用好" |
| **DT vs UEFI** | 两种向内核描述硬件的机制。ARM 世界主流是 **Device Tree**（静态编译的 DTB 由 bootloader 传给内核）；x86 是 **UEFI + ACPI** |
| **LSB / POSIX** | 嵌入式常常"不完整 POSIX"——BusyBox 是裁剪版，别指望桌面 Linux 那套工具链齐全 |

---

## HFT / 嵌入式关联

- **延迟量级**：Linux 的调度抖动是**尾延迟**问题（p99.9 刺尖），RTOS 是**上限保证**。HFT 生产环境走 x86 + 内核旁路（[13-dpdk](../../13-dpdk)），Pi 5 上练的是**思想**（绑核、中断隔离、缓存友好），不是生产指标。
- **SoC 视角很重要**：因为外设控制器在片内，驱动面对的是 **MMIO 寄存器** + **设备树节点**，而不是"插在总线上的卡"。这决定了后面 [03-platform-dt](../../09-device-drivers-dt/03-platform-dt) 为什么是驱动的核心。

---

## 本节笔记

| # | 笔记 |
|---|------|
| 1.1 | [Linux vs RTOS](./1.1-linux-vs-rtos.md) |
| 1.2 | [LSB vs POSIX](./1.2-lsb-vs-posix.md) |
| 1.3 | [把注意力放在 SoC 上](./1.3-focus-on-soc.md) |
| 1.4 | [树莓派是 SoC，不是 CPU](./1.4-raspberry-pi-is-soc.md) |
| 1.5 | [BSP 是模板不是产品](./1.5-bsp-is-template-not-product.md) |
| 1.6 | [UEFI ≈ U-Boot，不是 DTS · ACPI 与 DTB 才是对位关系](./1.6-device-tree-vs-uefi.md) |

---

## 验收

- [ ] 能说清"为什么飞控用 RTOS、网关用 Linux"各一条理由
- [ ] 看到 BCM2712 框图，能指出 CPU 核 / 内存控制器 / 外设各在哪
- [ ] 知道树莓派的 DTB 是谁生成、谁加载、内核怎么用
- [ ] 说出 BSP 交付后你还要自己做什么

---

## 衔接

- **下一步：** [02-toolchain](../02-toolchain/) — 装交叉工具链
- **卡住查书：** Primer Ch1–3、Ch7（见 [_refs/BOOK-MAP.md](../_refs/BOOK-MAP.md)）
