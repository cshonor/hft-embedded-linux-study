# Harris · Digital Design and Computer Architecture — **ARM 第2版（你手中的目录）**

> **唯一外文数字电路主力** — 打通数字逻辑 → CSAPP Ch4 → 嵌入式 Linux / STM32 + HFT。  
> 父章：[CSAPP Ch4 导读](../../README.md) · [数字电路资料总表](../section-补充-数字电路学习资料.md)

## 版本钉死（避免和别的印刷章号打架）

你梳理的是 **ARM 第2版 · 8 章结构**（Ch5=ARM ISA，Ch7=微处理器/流水线，Ch8=RPi）。  

部分英文 **ARM Edition（Elsevier）** 章序不同（例如 Digital Building Blocks、Architecture、Microarchitecture、Memory、I/O 拆开编号）。**以你手里书的目录为准**；下表按你的口述锁定。

| 章 | 主题（你的第2版） | 档位 |
|----|-------------------|------|
| **1** | 二进制基础、抽象/约束、进制、有符号数、逻辑门/CMOS | 浅读 |
| **2** | 组合逻辑：布尔、译码器、**MUX**、加法器、**ALU**；（书中含 Verilog 组合 — **跳过或扫一眼**） | **精读硬件** |
| **3** | 时序：锁存/触发器、寄存器堆、计数器、同步设计；（Verilog 时序/FSM — **跳过或浅读**） | **精读硬件** |
| **4** | HDL：Verilog 语法/层次/可综合建模 | **整章跳过** |
| **5** | **ARM 架构**：寄存器、指令格式、寻址、Thumb — 嵌入式 Linux 硬件地基 | **精读** |
| **6** | **存储器 + I/O**：层次/Cache/总线；GPIO/UART/SPI 编程模型 | Cache **精读**；外设 **中读** |
| **7** | **微处理器设计**：单周期→多周期→流水线；冒险/异常 ≈ CSAPP SEQ/PIPE | **精读 · 最高价值** |
| **8** | **Raspberry Pi 实战**：自定义外设、Linux 驱动、环境 — 理论落地 | 浅读/选做 |

## 为何有用（软硬件打通）

| 方向 | 收获 |
|------|------|
| **嵌入式 Linux / 驱动** | 数字逻辑 + ARM + MMIO/中断 → 懂内核/驱动「为什么」，不只对着手册改位 |
| **HFT** | 通路延迟、流水线冒险、Cache → 从底层看交易链路延迟从哪来 |
| **CSAPP Ch4** | Ch2/3/7 与 Y86 SEQ/PIPE 同语言；回刷框图不再抽象 |
| **RPi（Ch8）** | 把前面章节接到真实板子与驱动骨架（不替代完整驱动课） |

## 目录

```
digital_logic_harris_arm/
├── chapter_notes/   # ch1…ch8（按第2版）
├── cross_ref/
├── lab_logisim/
└── summary/
```

## 必抠小节（硬件 / 体系；HDL 一律旁路）

| 章 | 抓什么 | 放过什么 |
|----|--------|----------|
| Ch2 | MUX、译码器、加法器、ALU、传播延迟直觉 | Verilog 组合建模大段 |
| Ch3 | D-FF、寄存器堆、同步时序、setup/hold | Verilog 时序 / FSM 深挖 |
| Ch5 | ARM 寄存器、指令、寻址、Thumb 特点 | 不必背全编码表 |
| Ch6 | Cache 原理与映射；总线/外设模型扫清 | 不在此章写完整驱动 |
| Ch7 | 单周期数据通路；流水线；**冒险/转发/stall**；异常 | — |
| Ch8 | 看一遍「理论如何落到板子+驱动」 | 可不全做实验 |

## 学习节奏

1. **Ch2（硬件）+ Logisim MUX/加法器**  
2. **Ch3（硬件）+ Logisim 寄存器**  
3. **Ch5 ARM ISA** → `cross_ref/stm32_hardware.md`  
4. **Ch7 微处理器** → 写满 `cross_ref/csapp_ch4_link.md` + `hft_x86_timing.md`  
5. **Ch6 Cache** 精读；外设中读  
6. **Ch8** 选做；**回刷 CSAPP Ch4**  
7. **Ch4 整章不读**；Ch2/3 里的 Verilog 小节跳过

## 笔记规范

```markdown
> **Core Concept:** …
> **Link Target:** CSAPP §x.x / embedded Linux … / HFT …
```

## 一句话

第2版 ARM：逻辑（1–3）→ 跳过 HDL（4）→ **ARM（5）** → **存储/I/O（6）** → **CPU 实现与流水线（7）** → **RPi 串起来（8）**。主线 **2/3/5/6(cache)/7**；对照 CSAPP 盯 **Ch7**。
