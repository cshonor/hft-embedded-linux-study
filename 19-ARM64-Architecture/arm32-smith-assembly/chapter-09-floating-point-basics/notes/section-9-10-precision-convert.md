## §9.10 半精度与单精度转换 — VCVTB · VCVTT

> **Ch 9 · 浮点简介** · [章导读](../README.md)

---

### 为何半精度 (FP16)

| 收益 | 代价 |
|------|------|
| **存储减半** — 大数组、神经网络权重 | 精度/范围更低 |
| 总线/Flash 带宽 | 算前 often **转 single** |

**M4：** 硬件 **single 为主** — half 多用于 **load/store 压缩**。

---

### 转换指令（书中）

| 指令 | 方向（概念） |
|------|--------------|
| **`VCVTB`** | **B**ottom — single → half 存到 **寄存器低 16** 或内存半字 |
| **`VCVTT`** | **T**op — 类似，**高 16** 半字路径（成对打包） |

**用法模式：** 两个 half 可 **打包进一个 32 bit 字** — 类似 SIMD 打包思想。

**具体语法** 以 ARM 汇编手册为准 — 笔记记 **「single ↔ half 硬件转换，省软件 bit-twiddle」**。

---

### 与 [Ch22 FP16/NEON](../../chapter-22-fp-neon/) 

应用处理器 **NEON FP16** 算力更强 — M4 以 **VCVT + 存储** 为主。

---

### 可复述要点

1. **Half = 省内存**；计算常用 **promote to float**。  
2. **VCVTB/VCVTT** = M4 硬件 half↔single。  
3. 别手写 shift 除非无 FPU。
