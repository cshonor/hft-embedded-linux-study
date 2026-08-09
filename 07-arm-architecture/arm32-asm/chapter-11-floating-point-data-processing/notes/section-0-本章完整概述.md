## Ch11 完整概述 · 浮点数据处理指令

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Floating-Point Data-Processing Instructions · **跳过** / M4F **选读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **V* 算术** | ADD/SUB/MUL/DIV/SQRT · ABS/NEG |
| **比较接分支** | **VCMP → VMRS → B/IT** |
| **MAC 精度** | **VMLA（双舍入）vs VFMA（融合）** |
| **综合** | 二分法 + 泰勒 sin |

**前置：** [Ch9](../chapter-09-floating-point-basics/notes/section-0-本章完整概述.md) · [Ch10](../chapter-10-floating-point-rounding-exceptions/notes/section-0-本章完整概述.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **语法** | §11.2 | [§11.2](./section-11-2-syntax.md) |
| **指令表** | §11.3 | [§11.3](./section-11-3-summary.md) |
| **VCMP/VMRS** | §11.4 | [§11.4](./section-11-4-flags.md) |
| **FZ/DN** | §11.5 | [§11.5](./section-11-5-special-modes.md) |
| **VABS/VNEG** | §11.6 | [§11.6](./section-11-6-non-arithmetic.md) |
| **算术/MAC** | §11.7 | [§11.7](./section-11-7-arithmetic.md) |
| **二分/泰勒** | §11.8 | [§11.8](./section-11-8-examples.md) |

---

### 三、知识流（口述版）

```
Vop.F32 Sd,Sn,Sm — 无布尔，逻辑回整数 r
        ↓
VCMP/VCMPE → FPSCR；VMRS → APSR → B/IT
        ↓
FZ/DN 可选（FPSCR）
        ↓
VADD…VDIV/VSQRT；VMLA vs VFMA
        ↓
综合：二分 + sin 泰勒 — BL 子程序
        ↓
Ch12 表 / Ch13 正式 APCS
```

---

### 四、Ch9–11 浮点链（M4F 选读）

| 章 | 内容 |
|----|------|
| **9** | 格式 · CPACR · VLDR/VCVT |
| **10** | 舍入 · 五异常 · 结合律 |
| **11** | **算与比** |

**主线跳过三章** → [Ch13](../chapter-13-subroutines-stacks/) / [Ch16](../chapter-16-memory-mapped-peripherals/)

---

### 五、下一章

→ **[Ch12 表](../chapter-12-tables/)**（选读）
