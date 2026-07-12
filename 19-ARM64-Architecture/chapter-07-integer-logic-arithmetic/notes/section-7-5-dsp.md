## §7.5 DSP 扩展

> **Ch 7 · 整数逻辑与算术** · [章导读](../README.md)  
> **标签：** **选读** — Cortex-M4 **DSP 指令**；Linux/飞控有 NEON/FPU 时可压缩

---

### 定位

Cortex-M4 在 **整数 ALU** 上增加 **SIMD-ish / MAC** 类指令，加速：

- 音频 FIR/IIR  
- 简单视频 **运动估计**  
- 传感器融合（无 FPU 时）

**与 §7.4 饱和：** 同一 **Q 标志** 生态；饱和 + MAC 常一起出现。

---

### 典型指令（书中举例）

| 指令 | 作用 |
|------|------|
| **`SMMLAR`** 等 | **Signed Most Significant Word Multiply Accumulate with Rounding** — 带舍入的乘累加 |
| **`USADA8`** | **Unsigned Sum of Absolute Differences Accumulate (8-bit)** — 4 对字节 **绝对差** 累加 |

**USADA8 场景：** **MPEG / H.264** 等 **运动估计** — 块匹配算 SAD（Sum of Absolute Differences）。

```
两个 32 bit 寄存器各含 4 字节
    → 逐字节 |a[i]-b[i]| 相加累加
```

---

### 与飞控 / HFT 支线

| 路径 | 建议 |
|------|------|
| **MCU 无 FPU** | Q 格式 + DSP 指令做 PID/滤波（→ §7.7 · [24](../../../24-Motion-Control-Motor/)） |
| **Cortex-A + Linux** | **NEON** / 浮点 in C — 少手写 USADA8 |
| **HFT** | 定点 rarely；但 **饱和/clamp** 思想同 **risk limit** |

---

### 可复述要点

1. **M4 DSP** = 在整数管线上 **MAC、SAD** 等批量小数据运算。  
2. **USADA8** 服务 **视频块匹配** 类算法。  
3. 应用处理器路线 **选读** — 知道有即可。
