## §11.4 标志位 — VCMP · VCMPE · VMRS

> **Ch 11 · 浮点数据处理** · [章导读](../README.md)

---

### 浮点算术 **不** 设置 N,Z,C,V

与整数 **`ADDS`** 不同：

```asm
        VADD.F32    s0, s1, s2      ; ✗ 无 VADDS 设 APSR 用法
```

**VADD/VMUL/… 不写 FPSCR 比较标志** — 不能 **`BNE` 紧跟 VADD** 判大小。

---

### 比较指令 — `VCMP` / `VCMPE`

```asm
        VCMP.F32    s0, s1          ; 比较 s0 与 s1 → 更新 **FPSCR** N,Z,C,V
        VCMPE.F32   s0, #0.0        ; 可能 **额外** 对 NaN 等触发 Invalid（E=exception）
```

| | VCMP | VCMPE |
|---|------|-------|
| 更新 FPSCR 标志 | ✓ | ✓ |
| 无效操作 trap 倾向 | 较 quiet | **E** 版更严格（实现相关） |

**比较结果编码（概念）：** LT/EQ/GT/UN（**NaN 无序**）→ N,Z,C,V 组合 — 与整数 **不同编码表**。

---

### VMRS — 把 FPSCR 标志拷到 APSR

```asm
        VCMP.F32    s0, s1
        VMRS        APSR_nzcv, FPSCR   ; 浮点标志 → 整数条件执行可见
        BGT         greater            ; 或 ITGT / VMOVGT 等
```

| 步骤 | 原因 |
|------|------|
| **VCMP** | 只改 **FPSCR** |
| **VMRS APSR_nzcv, FPSCR** | **Ch8 条件分支/IT** 读 **APSR**，不直接读 FPSCR |

**易漏：** 比较 float 后 **忘记 VMRS** → 分支条件 ** stale 整数标志** — 经典 bug。

---

### 与 Ch8 衔接

```
VCMP → VMRS → B{cond} / IT{cond} / VMOV{cond}
```

**NaN：** `VCMP` 与 NaN → **UNordered** — 条件分支行为需查 ARM 条件码表（通常 **NE/EQ 组合** 判 unordered）。

---

### 可复述要点

1. **只有 VCMP/VCMPE 设浮点比较标志** — 在 FPSCR。  
2. **分支前必须 VMRS** 到 APSR（若用 ARM 条件码）。  
3. **NaN 比较** 走 UN — 控制环要 **显式 isnan** 防护。
