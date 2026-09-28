## §11.7 算术指令 — 加减 · 除 · 根号 · MAC

> **Ch 11 · 浮点数据处理** · [章导读](../README.md)

---

### 基础四则 + 根号

| 指令 | 操作 | 异常提示（Ch10） |
|------|------|------------------|
| **`VADD` / `VSUB`** | ± | IXC 常见；抵消（§10.5） |
| **`VMUL`** | × | OFC/UFC/IXC |
| **`VDIV`** | ÷ | **DZC**（非零/0）· IXC |
| **`VSQRT`** | √ | 负数 → **IOC** NaN |

**Ch3 示例：** `VADD.F32 s0, s1, s2` — 本章展开异常与 MAC。

---

### 链式 MAC vs 融合 FMA（核心精度差异）

**目标：** 计算 **acc + a×b**（滤波/Dot/矩阵乘常见）。

### 链式 — `VMLA`（Multiply-Accumulate）

```
t = round(a × b)          ← **第一次舍入**
acc = round(acc + t)      ← **第二次舍入**
```

**两次舍入** — 旧 FPU 行为；误差累积更大。

### 融合 — `VFMA`（Fused Multiply-Add, IEEE 754-2008）

```
acc = round(acc + a×b)    ← 内部 **无限精度积**，**仅最终舍入一次**
```

| | VMLA 链式 | VFMA 融合 |
|---|-----------|-----------|
| 中间舍入 | **有** | **无** |
| 精度 | 较低 | **较高** |
| 极端 case | 可能 **无效零/误溢出** | 更稳 |
| C 对应 | 分开 `*`+`+` 常类似链式 | `fmaf()` / `-ffma` |

**M4 若支持 VFMA** — 科学计算/DSP **优先 VFMA**；legacy 库可能仍 VMLA。

**相关：** `VMLS`/`VFMS` — 乘减；`VFNMA` — negated FMA 等 — 见指令摘要表。

---

### 与 Ch10 异常联动

每次 **舍入** → 可能 **IXC**；**VDIV** → **DZC**；大 **VMUL** → **OFC** — 读 **FPSCR** 或假设 C 语义。

---

### 可复述要点

1. **VDIV/VSQRT** 最易 **DZC/IOC**。  
2. **VMLA：两次舍入；VFMA：一次舍入** — 面试/数值常考。  
3. 滤波 inner loop：**VFMA** 是精度与性能 sweet spot。
