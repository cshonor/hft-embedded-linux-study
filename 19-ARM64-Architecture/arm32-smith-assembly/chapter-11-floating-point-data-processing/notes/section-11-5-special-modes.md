## §11.5 特殊模式 — Flush-to-Zero · Default NaN

> **Ch 11 · 浮点数据处理** · [章导读](../README.md)

---

### 配置位置

**FPSCR** 控制位（→ [Ch9 §9.8](../chapter-09-floating-point-basics/notes/section-9-8-fpu-control.md)）— **FZ (Flush-to-Zero)** · **DN (Default NaN)**。

---

### Flush-to-Zero (FZ)

| 开启后 | 效果 |
|--------|------|
| **输入** subnormal | 当作 **signed zero** |
| **输出** 本应 subnormal | **冲洗为 ±0** |

**目的：** **避免 denormal 慢路径** — 音频/DSP 常开；**牺牲极小值精度**。

**与 Ch10 UFC/下溢：** 更易得 **0** 而非 gradual underflow — **IXC** 行为仍查手册。

**HFT 对照：** 类似 **绝不碰 denormal tick** — 确定性/速度优先。

---

### Default NaN (DN)

| 开启后 | 效果 |
|--------|------|
| **任一操作数为 NaN** | 结果 = **固定 default qNaN**（payload **全 0**） |
| **不保留** | 原始 NaN 的 **payload 信息** |

**目的：** 简化硬件 NaN 传播 — **调试丢失 NaN 来源**。

**生产建议：** 通常 **DN 关** — 除非明确要简化语义。

---

### 可复述要点

1. **FZ** = subnormal **当 0** — 快但丢极小值。  
2. **DN** = 所有 NaN 运算 → **同一种 qNaN**。  
3. 科学/飞控 debug：**慎用 DN**；性能敏感 DSP：**可能开 FZ**。
