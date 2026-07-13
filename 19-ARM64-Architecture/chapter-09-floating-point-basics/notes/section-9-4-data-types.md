## §9.4 浮点数据类型 — IEEE 754 组成

> **Ch 9 · 浮点简介** · [章导读](../README.md)

---

### Cortex-M4 FPU 支持的格式

| 开发者名 | IEEE | bit | byte | 存储粒度 | 寄存器 | M4 用途 |
|----------|------|-----|------|----------|--------|---------|
| **f32** | binary32 | 32 | 4 | **word** | **s0–s31** | **主要硬件计算** |
| （半精度） | binary16 | 16 | 2 | halfword | 经转换 | **省 RAM**，精度低 |
| **f64** | binary64 | 64 | 8 | 双字（4 对齐） | **d0–d15** | M4 **常不硬件算** — 见实现 |

**Ch2 映射：** [§2.2 标量与浮点](../../chapter-02-programmers-model/notes/section-2-2-data-types.md) — **f32 与 s32/u32 同占 4 byte**，解释与 **r vs s 寄存器** 不同。

---

### 单精度位域（符号-幅度 + 偏置指数）

```
| S |   exp (8)   |      fraction f (23)      |
 1       bias=127           隐含 leading 1
```

**正常值公式：**

\[
F = (-1)^s \times 2^{(exp - 127)} \times 1.f
\]

| 字段 | 含义 |
|------|------|
| **S** | 0 正 · 1 负 |
| **exp** | 存储值 = 真实指数 + **127**（bias） |
| **f** | 小数部分；**规格化数隐含前导 1**（`1.f`） |

**Ch1 回顾：** [§1.5](../../chapter-01-overview-computing-systems/notes/section-1-5-representation.md) 已画框图 — 本章落到 **M4 指令**。

---

### 与整数比特

同一 32 bit 字 — **解释不同**：

```asm
        VMOV    r0, s0          ; 位拷贝，非「转换」（§9.9）
        VCVT    …               ; 真转换（§9.11）
```

---

### 可复述要点

1. **f32 = 32 bit word 宽 · f64 = 64 bit 双字** — 与 [Ch2 §2.2](../../chapter-02-programmers-model/notes/section-2-2-data-types.md) 整数表并列记忆。  
2. **M4 主战场 = f32 / s 寄存器**；**8 exp + 23 frac + sign**，bias **127**。  
3. **f64 / d*** 多为 **A 核完整 VFP** 或 **软件 double**；M4 以 **single** 为主。
