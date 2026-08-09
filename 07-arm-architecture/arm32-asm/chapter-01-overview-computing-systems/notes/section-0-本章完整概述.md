# Ch1 完整精读总结 · 计算机系统概述

> ***ARM Assembly Language Fundamentals and Techniques*** — Hohl & Hinds · **选读**  
> English: *An Overview of Computing Systems*  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)

---

## 本章定位

全书**前置基础**：硬件模型、RISC、ARM 起源、数制与数据表示、工具链。  
**不写汇编指令**（从 Ch3 起）；为 Ch2 程序员模型铺垫。  
有 CSAPP / C 基础可压缩：盯住 **Load/Store**、**补码**、**Cortex-A/R/M**、**工具链流水线**。

---

## §1.1 / §1.3 · 存储程序模型与计算设备

三大件：**CPU**（算与控）· **内存**（指令区 + 数据区）· **总线**（地址/数据）。  
手机、MCU、树莓派、工控板都遵循；嵌入式常无键鼠屏，靠传感器入、外设出。

硬件层级（书中自上而下）：

```
应用 / OS
  → C/C++
  → 汇编（本书主战场）
  → 处理器微架构
  → 逻辑门
  → 晶体管
```

→ 细节：[§1.1](./section-1-1-intro.md) · [§1.3](./section-1-3-computing-devices.md)

---

## §1.2 · RISC vs CISC · ARM 史 · Cortex 三线

### CISC 痛点（VAX、8086、68000 等）

变长指令、译码重；单指令可「访存+运算」；微码占面积功耗；复杂指令使用率低。

### RISC 准则（伯克利 / 斯坦福脉络）

1. **定长**指令、译码简单  
2. 基础指令倾向单周期 / 流水线友好  
3. **仅 Load/Store 访存** — 加减移位只碰寄存器（全书核心习惯）  
4. 少/无微码，硬连线实现  
5. 指令集精简、易验证  
6. 代价：复杂任务指令条数多，靠频率与流水线补  

### ARM 简史

Acorn → ARM1（Furber / Wilson，1985）→ 1990 ARM Ltd（**卖 IP 授权，不卖芯片**）→ ARM7TDMI 里程碑。

### Cortex 三线（必背）

| 系列 | 场景 | 要点 |
|------|------|------|
| **Cortex-A** | 手机、平板、树莓派等 | **有 MMU**，跑 Linux/Android — 本书**只科普** |
| **Cortex-R** | 汽车、医疗等 | 硬实时 — 本书**只对比** |
| **Cortex-M** | 家电、传感器 MCU | 常无 MMU；**全书实操主线** |

**讲 A、不练 A：** [CORTEX-A-SCOPE.md](../../CORTEX-A-SCOPE.md)。本目录主练 **v4T / v7-M**；Pi5 Linux → [奔跑吧 AArch64](../../../aarch64-practice/)。

→ [§1.2](./section-1-2-risc-history.md)

---

## §1.4 · 进制

| 进制 | 用途 |
|------|------|
| 十进制 | 日常 |
| 二进制 | 硬件 0/1 |
| 十六进制 | 调试/手册；**4 bit = 1 hex 位** |

例：`110101₂` = 53₁₀；`0xA5E9` 按 16 的幂展开。

→ [§1.4](./section-1-4-number-systems.md)

---

## §1.5 · 整数 · 浮点 · ASCII

### 整数

| 编码 | 现状 |
|------|------|
| 符号幅值 | 少用 |
| 反码 | ±0、修正开销 |
| **补码** | **行业标准**；加减无需特殊修正 |

范围备忘：8 位 −128…127；16 位 −32768…32767；32 位约 ±2.1×10⁹。

### 浮点 IEEE754

符号 + 指数（含偏置）+ 尾数；单/双精度；特殊值：±0、非规格化、∞、NaN。  
无 FPU 的 MCU → 定点模拟（Ch9–11 可后读）。

### ASCII

1 字节字符/控制符；汇编可用 `#'A'`（→ `0x41`）。

→ [§1.5](./section-1-5-representation.md)

---

## §1.6 · 助记符 ↔ 机器码

机器码 = 比特串；**MOV/ADD/LDR** = 助记符，汇编器翻译。  
**ARM / x86 / DSP 编码互不兼容** — 换架构必须重编。

→ [§1.6](./section-1-6-bits-to-commands.md)

---

## §1.7 · 工具链

| 路线 | 代表 | 本仓库建议 |
|------|------|------------|
| 开源 | Linaro/GNU **GCC + GDB** | Pi / Linux：**主用** |
| 商业 | Keil MDK、TI CCS | MCU 课可用；附录 A/B **可跳** |

流水线：

```
源（C/asm）→ 编译/汇编 → .o → 链接 → ELF → 调试/烧录
```

反汇编、内存窗、断点 = 日常调试三件套。

→ [§1.7](./section-1-7-tools.md)

---

## §1.8 · 习题考点清单

- [ ] 二 / 十 / 十六互转  
- [ ] 8/16 位原码、反码、**补码**  
- [ ] RISC vs CISC（尤其 **Load/Store**）  
- [ ] Cortex-**A / R / M** 场景  
- [ ] 存储程序三大件  
- [ ] 简单二进制乘法（若书中布置）  

→ [§1.8](./section-1-8-exercises.md)

---

## 知识流（口述）

```
选 RISC：定长 + 仅 Load/Store
  → 冯·诺依曼：指令与数据都在内存
  → 比特表示：hex、补码、IEEE754、ASCII
  → 助记符 → .o → 链接 → 可执行（GNU 或 Keil/CCS）
  → Ch2：落在哪些寄存器、哪种模式
```

---

## 与仓库对照

| 模块 | 呼应 |
|------|------|
| [CSAPP](../../../../02-computer-systems/) | 另一 ISA，同一「表示」层 |
| [01 C](../../../../01-c-language/) | 补码、类型宽度 |
| [Primer SoC](../../../../08-embedded-boot-build/primer-system-overview/chapter-03-processor-basics/) | Cortex-A SoC 跑 Linux |
| [aarch64-practice](../../../aarch64-practice/) | Pi5 主战场 |

---

## 下一章

→ **[Ch2 程序员模型](../../chapter-02-programmers-model/)**（**精读**）— ARM7TDMI vs Cortex-M4 寄存器、模式、向量表
