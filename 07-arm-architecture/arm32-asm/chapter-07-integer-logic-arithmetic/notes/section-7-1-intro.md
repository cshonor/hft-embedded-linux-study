## §7.1 简介

> **Ch 7 · 整数逻辑与算术** · [章导读](../README.md)

---

### 本章在全书中的位置

| | |
|---|---|
| **角色** | **精读** — 全书 **篇幅最长、最核心** 之一；Load 进寄存器后的 **全部整数 ALU** |
| **前置** | [Ch5 Load/Store](../../chapter-05-loads-stores-addressing/notes/section-0-本章完整概述.md) · [Ch6 常数进寄存器](../../chapter-06-constants-literal-pools/notes/section-0-本章完整概述.md) · [Ch1 补码](../../chapter-01-overview-computing-systems/notes/section-1-5-representation.md) |
| **后续** | **Ch8** 用 **N/Z/C/V** 做条件分支 · **Ch13** 乘除在函数里 · **Ch24** 定点/Q 格式与飞控 |

---

### 七主题骨架

```
§7.2  N · Z · C · V · S 后缀 · Q（饱和）
§7.3  CMP · CMN · TST · TEQ
§7.4  逻辑 · 桶形移位 · 加减 · ADC/SBC · 乘 · 除 · 饱和 SSAT/USAT
§7.5  DSP 扩展 — SMMLAR · USADA8 等
§7.6  BFI · UBFX/SBFX · BFC · RBIT（M4）
§7.7  Q 格式定点
```

---

### 阅读策略

| 块 | 建议 |
|----|------|
| **§7.2–7.4** | **必做** — 驱动、内核、协议解析都靠这些 |
| **§7.5 DSP** | 选读 — 音频/视频 MCU；飞控有 FPU/NEON 时可后补 |
| **§7.6 位域** | 精读 — 寄存器 bitfield 与 DT 掩码同源 |
| **§7.7 Q 格式** | 选读 — 无 FPU 时姿态/PID 定点；有 float 可压缩 |

---

### 可复述一句话

> Ch7 = **寄存器里的整数怎么算、标志位怎么变** — 后面所有 **if/循环/64 位/定点** 都站在这上面。
