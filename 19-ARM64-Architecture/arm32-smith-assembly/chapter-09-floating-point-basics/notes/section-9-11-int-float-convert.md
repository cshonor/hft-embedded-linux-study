## §9.11 浮点与整数/定点格式相互转换 — VCVT

> **Ch 9 · 浮点简介** · [章导读](../README.md)

---

### 整数 ↔ 单精度 — `VCVT`

```asm
        VCVT    s0, r0                ; 有符号/无符号变体见手册 VCVT.xxx
        VCVT    r0, s0, #0            ; float → int，向零舍入（截断小数）
```

| 方向 | 行为 |
|------|------|
| **int → float** | 精确表示整数（在 float 可表示范围内） |
| **float → int** | 默认 **toward zero** — `(int)f` 截断；溢出 → 饱和或异常（FPSCR） |

**与 VMOV 区别：**

```
VMOV s0, r0   →  位模式拷贝（垃圾 int 当 float 解释）
VCVT s0, r0   →  数值 42 → 42.0f
```

---

### 定点 (Fixed-point) ↔ float

**`VCVT` 带 `#fbits`（书中）：** 指定 **小数位数** 的定点 interpreted 整数 ↔ single。

| 场景 | 说明 |
|------|------|
| **ADC 采样** | 驱动读 12 bit 左对齐 — 转 engineering unit float |
| **DAC 输出** | float PID 输出 → 定点 → 寄存器 |
| **vs Ch7 Q 格式** | 硬件 **VCVT** 省 **shift + SSAT** 软件 |

**规则回顾（Ch7.7）：** Qn×Qm → 乘积需 **rescale**；**VCVT** 把 rescale 放进 FPU。

---

### C 对照

| C | 汇编倾向 |
|---|----------|
| `(float)i` | VCVT |
| `(int)f` | VCVT 向零 |
| `*(float*)&i` | VMOV — **类型双关，危险** |

---

### 可复述要点

1. **数值转换用 VCVT**；**位模式用 VMOV**。  
2. **float→int 截断** — 控制环取整时要注意 bias。  
3. **#fbits 定点 VCVT** — ADC/DAC 与 [24 飞控](../../../24-Motion-Control-Motor/) 接口实用。
