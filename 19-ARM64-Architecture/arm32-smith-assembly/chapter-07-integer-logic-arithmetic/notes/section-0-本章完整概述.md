## Ch7 完整概述 · 整数逻辑与算术

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Integer Logic and Arithmetic · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **ALU 全集** | 逻辑 · 移位 · 加减 · 乘除 · 饱和 — Load 之后的主战场 |
| **标志位** | **N Z C V**（+ **Q**）— **Ch8 条件执行** 的直接输入 |
| **M4 增值** | **UDIV/SDIV** · 位域指令 · DSP — 按路线选读 |

**前置：** [Ch5–6](../chapter-05-loads-stores-addressing/notes/section-0-本章完整概述.md) · [Ch1 补码](../../chapter-01-overview-computing-systems/notes/section-1-5-representation.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **标志位 · S 后缀** | §7.2 | [section-7-2-flags.md](./section-7-2-flags.md) |
| **CMP/CMN/TST/TEQ** | §7.3 | [section-7-3-compare.md](./section-7-3-compare.md) |
| **数据处理 · 饱和** | §7.4 | [section-7-4-data-processing.md](./section-7-4-data-processing.md) |
| **DSP 扩展** | §7.5 | [section-7-5-dsp.md](./section-7-5-dsp.md) |
| **BFI/UBFX/RBIT** | §7.6 | [section-7-6-bit-ops.md](./section-7-6-bit-ops.md) |
| **Q 定点** | §7.7 | [section-7-7-fractional.md](./section-7-7-fractional.md) |

---

### 三、知识流（口述版）

```
Load 数据到寄存器
        ↓
AND/ORR/EOR/BIC + 桶形移位
        ↓
ADD/SUB/ADC · MUL 或 shift-add · UDIV(M4)
        ↓
CMP/TST → N,Z,C,V
        ↓
（可选）SSAT/USAT · DSP MAC · UBFX 解析 MMIO
        ↓
（可选）Q 格式定点
        ↓
Ch8：B.cond / IT / 循环
```

---

### 四、与支线对照

| 场景 | Ch7 |
|------|-----|
| Ch3 阶乘 CMP | §7.3 |
| 内核 64 bit `jiffies` | ADC 链 |
| 驱动 bitfield | UBFX/BFI |
| [21 驱动](../../../../21-Linux-Device-Driver/) `readl`+掩码 | 同 TST/BFI 语义 |
| [24 PID/Kalman](../../../24-Motion-Control-Motor/) | Q 或 float |
| HFT 定点 tick | Q 思想 |

---

### 五、下一章

→ **[Ch8 分支与循环](../../chapter-08-branches-loops/)**（**精读**）
