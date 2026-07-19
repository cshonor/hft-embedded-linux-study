# Harris · Digital Design and Computer Architecture (ARM Edition)

> **唯一外文数字电路主力** — 打通数字逻辑 → CSAPP Ch4 → STM32 + HFT(x86) 直觉。  
> 不必再配其他外文教材；中文阎石/王红仅作可选快车道。  
> 父章：[CSAPP Ch4 导读](../../README.md) · [数字电路资料总表](../section-补充-数字电路学习资料.md)

## 为何只学这一本（ARM 版）

| 需求 | Harris ARM 如何覆盖 |
|------|---------------------|
| CSAPP §4.2～4.5 | 门 → MUX → ALU → 触发器 → 单周期 → 五级流水线 + hazards，与 Y86 SEQ/PIPE 同思路 |
| STM32 | ARM 版案例贴 Cortex 家族，对接 GPIO/时序直觉 |
| HFT / x86 | 流水线、转发、stall、cache 原理通用（上线写 x86，书里用 ARM 当教具） |

**选 ARM Edition，不选 RISC-V：** 嵌入式主线是 STM32；RISC-V 版更偏服务器 ISA 教具，对你适配略低。

## 目录

```
digital_logic_harris_arm/
├── chapter_notes/       # 逐章 Markdown（核心）
├── cross_ref/           # ↔ CSAPP / STM32 / HFT（最重要）
├── lab_logisim/         # 仿真记录与 assets/
└── summary/             # 全书速记
```

## 必学 / 浅读 / 跳过

| 档 | 章节 | 抓什么 |
|----|------|--------|
| **精读** | **Ch2** | §2.8 Building Blocks（**MUX**、译码器）；§2.9 Timing（传播延迟） |
| **精读** | **Ch3** | §3.2 锁存/D 触发器；§3.5 setup/hold（stall 根因） |
| **精读** | **Ch5** | §5.2 加法器/ALU；§5.5 Memory Arrays（SRAM） |
| **精读** | **Ch6** | §6.3 单周期≈SEQ；§6.4 五级+hazards≈PIPE；§6.5 cache 基础 |
| 浅读 | Ch1 | 二进制/CMOS — 懂电平即可，不抠半导体物理 |
| 浅读 | Ch7 | I/O、中断、GPIO 数字逻辑 — 扫一眼，不练驱动 |
| **跳过** | **Ch4** | Verilog/VHDL 整章 + FPGA/版图 — 软件底层不写 HDL |

## 学习节奏（只靠本书）

1. **Ch2 + Ch5** → MUX / 全加器 / ALU；Logisim 搭 2 选 1 MUX、多位加法器  
2. **Ch3** → D 触发器、流水线锁存直觉  
3. **Ch6** → 写满 `cross_ref/csapp_ch4_link.md`（对照 CSAPP §4.3～4.5）  
4. 每章后补 `cross_ref`（STM32 / HFT）  
5. 全书一遍后 **回刷 CSAPP Ch4** — 框图应能对上  

## 笔记规范

每篇 `chapter_notes/*.md` 开头固定两行：

```markdown
> **Core Concept:** …
> **Link Target:** CSAPP §x.x / STM32 … / x86 pipeline …
```

- 文件名英文；核心概念单独成段  
- 截图 / Logisim 图放对应章 `assets/`（如 `chapter_notes/assets/` 或 `lab_logisim/assets/`）

## 一句话

单独本文件夹只装 Harris ARM；严格按上表精读，跳过 HDL — **足够**打通数字电路 → CSAPP → 嵌入式 + HFT，无需再堆外文书。
