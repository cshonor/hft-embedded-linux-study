## Ch10 完整概述 · 浮点舍入与异常

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Introduction to Floating-Point: Rounding and Exceptions · **跳过** / M4F **选读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **舍入** | G/S · **RNE/RP/RM/RZ** · FPSCR RMode |
| **异常** | **五标志** + 默认 ∞/NaN/ subnormal |
| **数值稳定** | 结合律失效 · **抵消** |

**前置：** [Ch9 格式/FPSCR](../chapter-09-floating-point-basics/notes/section-0-本章完整概述.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **舍入模式** | §10.2 | [section-10-2-rounding.md](./section-10-2-rounding.md) |
| **五类异常** | §10.3 | [section-10-3-exceptions.md](./section-10-3-exceptions.md) |
| **代数定律** | §10.4 | [section-10-4-algebra.md](./section-10-4-algebra.md) |
| **规格化/抵消** | §10.5 | [section-10-5-normalization.md](./section-10-5-normalization.md) |

---

### 三、知识流（口述版）

```
内部高精度 → 23 frac 必舍入（G/S）
        ↓
RNE 默认；RZ 截断；RP/RM 区间算术
        ↓
越界/非法 → DZC/IOC/OFC/UFC + 默认结果
任何舍入 → IXC
        ↓
(A+B)+C ≠ A+(B+C)；相近相减 → 精度崩
        ↓
Ch11：VADD/VMUL 与 FPSCR 标志
```

---

### 四、路线对照

| 路径 | Ch10 最小集 |
|------|-------------|
| **Linux C** | NaN/∞/别乱 `-ffast-math` |
| **飞控 float** | §10.3–10.5 |
| **HFT int tick** | §10.4 对照 |

---

### 五、下一章

→ **[Ch11 浮点数据处理](../chapter-11-floating-point-data-processing/)**（跳过 / FPU 续）
