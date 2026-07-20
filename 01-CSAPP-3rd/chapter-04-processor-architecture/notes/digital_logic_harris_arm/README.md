# Harris · Digital Design and Computer Architecture — **ARM Edition**

> **唯一外文数字电路主力** — 打通数字逻辑 → CSAPP Ch4 → STM32 + HFT(x86) 直觉。  
> 父章：[CSAPP Ch4 导读](../../README.md) · [数字电路资料总表](../section-补充-数字电路学习资料.md)

## 版本说明

| 说法 | 实际 |
|------|------|
| 你说的「ARM 版 / 覆盖到 RPi」 | 指 **Sarah & David Harris《Digital Design and Computer Architecture: ARM Edition》** |
| 章数 | 正文常见 **Ch1–Ch8**，**I/O（含 Raspberry Pi 实践）多为 Ch9 / 配套网站章** — 合计「逻辑→ARM 处理器→内存→I/O 落地」一整条 |
| 易混 | 同系列 **MIPS《DDCA 2nd》** 章名类似，但 **ARM 版 Ch6=ISA、Ch7=微架构(流水线)** — **不要按旧笔记把流水线记成 Ch6** |

官方配套：https://pages.hmc.edu/harris/ddca/ddcaarm.html

## 为何只学这一本（ARM 版）

| 需求 | 覆盖 |
|------|------|
| CSAPP §4.2～4.5 | 门→MUX→ALU→触发器→单周期→流水线+hazards（主要在 **Ch7**） |
| STM32 | ARM ISA（**Ch6**）+ I/O 直觉（**Ch9/RPi**） |
| HFT / x86 | 冒险/缓存原理通用；书中 ARM 是教具，上线仍写 x86 |

## 全书地图（按 ARM Edition 真实目录）

| 章 | 英文名 | 你的档位 | 一句话 |
|----|--------|----------|--------|
| **1** | From Zero to One | 浅读 | 数制、门、数字抽象；不抠 CMOS 物理 |
| **2** | Combinational Logic Design | **精读** | **MUX**、译码、§2.9 Timing |
| **3** | Sequential Logic Design | **精读** | D-FF、寄存器、setup/hold |
| **4** | Hardware Description Languages | **跳过** | Verilog/VHDL — 不写 HDL |
| **5** | Digital Building Blocks | **精读** | 加法器/ALU、SRAM |
| **6** | Architecture | 精读偏 ISA / 可中读 | **ARM 指令集与程序员模型**（≠流水线实现） |
| **7** | Microarchitecture | **精读 · 最高价值** | 单周期 / 多周期 / **五级流水线 + hazards** ≈ CSAPP SEQ/PIPE |
| **8** | Memory Systems | **精读** | Cache、VM 基础 → HFT 局部性 |
| **9** / 配套 | I/O Systems (+ **Raspberry Pi**) | 浅读 | GPIO/外设/RPi 实践 — 理论落地，不深挖驱动工程 |

```
digital_logic_harris_arm/
├── chapter_notes/     # ch1…ch9 脚手架
├── cross_ref/         # ↔ CSAPP / STM32 / HFT
├── lab_logisim/
└── summary/
```

## 必学小节（对准 MUX / ALU / 流水线）

| 章 | 必抠 |
|----|------|
| **Ch2** | §2.8 Building Blocks（MUX）；§2.9 Timing |
| **Ch3** | §3.2 Latches/FFs；§3.5 Sequential timing |
| **Ch5** | §5.2 Arithmetic（ALU）；§5.5 Memory arrays |
| **Ch6** | ARM 汇编/机器模型 — 够读懂后面微架构即可 |
| **Ch7** | Single-cycle；**Pipelined + Pipeline Hazards**（转发/stall/flush） |
| **Ch8** | Caches（对照 CSAPP Ch6 / HFT） |

## 学习节奏

1. **Ch2 + Ch5** → MUX / ALU；Logisim  
2. **Ch3** → 触发器 / 锁存  
3. **Ch6** 扫 ARM ISA → **Ch7** 精读微架构 + 写满 `cross_ref/csapp_ch4_link.md`  
4. **Ch8** cache → `hft_x86_timing.md`  
5. **Ch9/RPi** 浅读 → `stm32_hardware.md`  
6. **回刷 CSAPP Ch4**

## 笔记规范

```markdown
> **Core Concept:** …
> **Link Target:** CSAPP §x.x / STM32 … / x86 pipeline …
```

截图放 `lab_logisim/assets/` 或章内 `assets/`。

## 一句话

ARM 版从数字逻辑一路讲到 **ARM 微架构、内存与 I/O（RPi）**；你的主线是 **Ch2/3/5 + Ch7/8**，跳过 Ch4 HDL，Ch6/Ch9 按需。流水线对照 CSAPP 请盯 **Ch7**，不是 Ch6。
