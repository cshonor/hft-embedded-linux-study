## §10.3 浮点异常 — DZC · IOC · OFC · UFC · IXC

> **Ch 10 · 浮点舍入与异常** · [章导读](../README.md)

---

### 与整数异常对比

| | 整数除零 | IEEE 754 浮点异常 |
|---|----------|-------------------|
| 典型行为 | **trap / fault** | **默认结果 + FPSCR 标志** — **程序常继续** |
| 目的 | 硬错误 | 可查询、可忽略、或可选 trap |

**FPSCR** 累积 **DZC / IOC / OFC / UFC / IXC**（及 **IDC** 等）— [Ch9 §9.8](../chapter-09-floating-point-basics/notes/section-9-8-fpu-control.md)。

---

### 五类异常

### 1. 被零除 (DZC — Division by Zero)

**非零** normal/subnormal **÷ 0** → 默认 **±∞**（符号规则），**DZC=1**。

与 **0/0**（Invalid）区分。

---

### 2. 无效操作 (IOC — Invalid Operation)

**无公认数学定义** 的操作，例如：

- **0 ÷ 0**  
- **∞ × 0**  
- **sqrt(-1)**（实域）  
- **float→int** 超出整数可表示范围  
- 操作数含 **sNaN**（发信 NaN）

**默认结果：** 常 **qNaN** 或 **最大整数**（视操作）— **IOC=1**。

---

### 3. 溢出 (OFC — Overflow)

结果 **绝对值太大** — 超出目标格式最大 **normal** 范围。

**默认 RNE：** 常返回 **±∞**。**必同时 IXC=1**（舍入到 ∞ 视为 inexact）。

---

### 4. 下溢 (UFC — Underflow)

结果 **绝对值太小** — 不能正常表示。

**默认：** 返回 **subnormal** 或 **±0**（视 **FTZ/FZ** 与舍入）。若仍 inexact → **IXC**。

**Ch9 Subnormal** — 与 **FZ _flush-to-zero**（Ch11）联动。

---

### 5. 结果不精确 (IXC — Inexact)

**任何舍入** 使 **存储结果 ≠ 数学无限精度结果** → **IXC=1**。

**最频繁** — 大量「正常」运算都会 inexact。

---

### 程序员应对

| 策略 | 说明 |
|------|------|
| **忽略标志** | 多数嵌入式 C 代码 — 默认 |
| **周期读 FPSCR** | 调试/计量 |
| **检查 NaN/Inf** | 控制环 **isnan/isinf** — [24 飞控](../../../24-Motion-Control-Motor/) |
| **启用 trap** | 少见 — 需 OS/ handler 支持 |

---

### 可复述要点

1. **五标志：DZC IOC OFC UFC IXC** — 写 FPSCR，**默认不杀进程**。  
2. **0/0→NaN，非零/0→∞** — 别与整数除零混淆。  
3. **OFC 常伴 IXC**；**下溢** 与 subnormal/FTZ 相关。
