# Embedded Linux Primer 第 03 章 — Processor Basics

> 对应目录：`chapter-03-processor-basics/`  
> 书：*Embedded Linux Primer*, 2nd ed — Christopher Hallinan（约 2010/2011，芯片清单偏当年）  
> 大纲：[../OUTLINE.md](../OUTLINE.md)

**优先级**：3.1 速览；3.2 SoC **选读（ARM 精读）**；3.3–3.4 按需  
**前置**：[Ch2 Big Picture](../chapter-02-big-picture/notes.md) · [U-Boot vs UEFI](../chapter-02-big-picture/2.1-uboot-bios-uefi.md)  
**后置**：[Ch4 内核工程视角](../chapter-04-kernel-construction/) · [Ch7 U-Boot](../chapter-07-bootloaders/) · [10 ARM](../../../10-arm-architecture/)

---

## 章节定位

硬件基础章：嵌入式 Linux 常见处理器分两大类——**独立 CPU + 芯片组** vs **集成 SoC**；并扫一眼电信/工控标准化背板（cPCI / ATCA）。  
承接 Ch2 架构图，为 U-Boot 移植、内核板级适配提供「板子上到底是什么硅」的背景。

**成书年代：** Power / MIPS / ARM / x86 产品线以 2011 前后主流为例；今日选型看现货 SoC 与厂商 BSP，**分类框架仍有效**。

---

## 3.0 硬前提：必须有 MMU

本章（及本书 Linux 主线）处理器均带 **MMU**。  
虚拟内存、用户/内核隔离依赖 MMU；无 MMU 的 8 位 MCU、多数 Cortex-M **不在本章范围**（与 [Linux vs RTOS](../chapter-01-introduction/1.1-linux-vs-rtos.md) 边界一致）。

---

## 3.1 Stand-Alone Processors（速览）

**定义：** 纯 CPU 核，外设少；须配 **北桥/南桥（或等价桥片）** 才成系统。算力偏强 → 高端通信、服务器刀片、工控 x86 板等。

| 书中例子 | 架构要点 | 当年场景 |
|----------|----------|----------|
| IBM 970FX | 64 位 Power，超标量、深流水；动态调频 / 低功耗态 | 高性能刀片 / 计算存储 |
| Intel Pentium M | x86 移动低功耗，多档变频 | 老式工控 x86 |
| Intel Atom | 低功耗 x86，二进制兼容桌面 x86 生态 | 上网本、小型网关（仍要芯片组） |
| Freescale MPC7448（G4/e600） | Power，高主频；AltiVec | ATCA 等电信板；音视频/信号处理 |

### 芯片组角色（南北桥叙事）

| 桥 | 典型管什么 |
|----|------------|
| **北桥** | 贴 CPU 前端总线：DRAM、高带宽外设（当年显卡等） |
| **南桥** | PCI、USB、IDE/存储、以太、低速 IO |

例：855GM ↔ Pentium M；Power 独立 CPU 常用专用桥片（书中 Tundra 等）。  
**今日：** 很多「独立 CPU」叙事已被 SoC / SiP 吃掉；读懂「CPU 与外设谁集成」即可。

---

## 3.2 Integrated Processors: SoC（选读 · ARM 精读）

**定义：** 单片集成 CPU + 内存控制器 + 以太/USB/UART/（常有）通信加速等 → 外围极简。  
消费与工业嵌入式的**绝对主流**。书中按 Power / MIPS / ARM 三条线举例（另加 x86 SoC 时代已在发展）。

### Power（Freescale 通信线为主）

PowerQUICC / QorIQ：通信设备标杆；常有 **CPM / 数据通路加速**，卸载报文，减轻主 CPU。

| 系列 | 书中量级特征 | 典型用途 |
|------|--------------|----------|
| PowerQUICC I（MPC8xx） | 50–133 MHz；多 SCC | ADSL / 小型家用网关 |
| PowerQUICC II（MPC82xx） | G2；百兆 / ATM；MCC | 中小型交换 |
| PowerQUICC II Pro（MPC83xx） | e300；千兆；安全引擎 | 路由 / 工业网关 |
| PowerQUICC III（MPC85xx） | e500；可达 ~1.5 GHz；RapidIO | 基站、光传输 |
| **QorIQ**（P1/P2/P4…） | 多核 e500mc；虚拟化、DPAA 等 | 高端电信 / 5G 侧设备叙事 |
| AMCC PPC440 等 | FPU、双千兆等 | 工控 / 存储；书中部分开发板案例 |

### MIPS

授权 RISC；当年家用路由、机顶盒常见（Broadcom 等）：

- Broadcom SiByte：单/双/四核 MIPS64，低功耗路由 / 机顶盒  
- Cavium Octeon 等：多核网络处理  
- 其他机顶盒芯片（书中 ATI 等）

### ARM（消费 / 多媒体 — 本仓库精读方向）

| 线 | 要点 | 书中关联 |
|----|------|----------|
| TI **OMAP** | Cortex-A + DSP；LCD / Camera / SD | BeagleBoard（OMAP3530）类案例 |
| Freescale / NXP **i.MX** | ARM9/11 → Cortex-A；编解码 | 车载、工业 HMI |
| 三星等 | 大量消费 SoC | 家电、便携 |

深挖 ISA / 异常 / 启动：→ [10-arm-architecture](../../../10-arm-architecture/)。  
板级今日例：Pi5 = 应用核 SoC + **RP1** 外设（见 [Pi Labs](../../../13-embedded-projects/RASPBERRY-PI5-LABS.md)）。  

**FAQ：树莓派算哪一类？** → [3.2-raspberry-pi-is-soc.md](./3.2-raspberry-pi-is-soc.md)（结论：**SoC，不是独立处理器**）。

---

## 3.3 Other Architectures（按需）

内核 `arch/` 下架构远多于书中四条主线；SPARC、Xtensa 等书中一笔带过。嵌入式产品选型极少主动跟，**知道「内核可移植、不等于市场主力」即可。**

---

## 3.4 Hardware Platforms：标准化背板（按需）

讲的是**整机/刀片标准**，不是某一颗 CPU：

| 平台 | 特征 | 场景 |
|------|------|------|
| **CompactPCI (cPCI)** | 欧卡；热插拔；3U/6U | 老式工控 |
| **ATCA**（PICMG 3.x） | 冗余电源/散热、高速交换背板 | 运营商基站、核心交换等 COTS |

目标：少自研整机结构，买标准槽位 + 板卡。与「自制单板 + U-Boot」是另一条供应链故事。

---

## 3.5 小结

1. 两类：**独立 CPU + 桥片**（高性能）vs **SoC**（嵌入式主流）  
2. 书中四条 Linux 常见线：**Power（通信）· MIPS（路由/机顶盒）· ARM（消费多媒体）· x86（工控/小网关）**  
3. 电信看 PowerQUICC/QorIQ 叙事；消费看 ARM；家用网络曾看 MIPS  
4. cPCI / ATCA 降整机自研成本（高可靠机房）  
5. **一律要 MMU** 才进本书 Linux 主线  

### 与前后章 / 仓库的咬合

| 点 | 落到 |
|----|------|
| 这些板的 Bootloader | 多为 **U-Boot**（≠ PC BIOS/UEFI）→ [2.1 FAQ](../chapter-02-big-picture/2.1-uboot-bios-uefi.md) · [Ch7](../chapter-07-bootloaders/) |
| 编内核时架构 | `ARCH=powerpc` / `arm` / `mips` / `x86` 等 → [Ch4](../chapter-04-kernel-construction/) |
| 书中移植案例板 | Freescale / AMCC / TI 等 → 读到案例时回对照本节芯片族 |
| 你现在的板 | Pi5 / 现代 ARM SoC；芯片名变了，**SoC + DT + 交叉编译** 框架不变 |

**下一章：** [Ch4 Kernel Construction](../chapter-04-kernel-construction/notes.md)（工程视角编内核，非 LKD 原理）。

---

## 参考

- Hallinan, *Embedded Linux Primer*, 2nd ed, Chapter 3  
- 大纲：[../OUTLINE.md](../OUTLINE.md) §第 3 章
