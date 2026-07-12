## §3.6 示例程序 4 — 浮点数操作 (FPU)

> **Ch 3 · 指令集简介** · [章导读](../README.md)  
> **标签：** **选做** — 嵌入式 Linux 主线可跳过（见 [OUTLINE](../../OUTLINE.md) Ch9–11）

---

### 本程序学什么

| 目标 | 内容 |
|------|------|
| **启用 FPU** | 写 **CPACR** 打开 CP10/CP11 |
| **浮点寄存器** | `s0–s31` 单精度 |
| **基础浮点指令** | `VMOV.F` · `VADD.F` |

**硬件：** **Cortex-M4F**（带 F 后缀）；无 FPU 的 M4 不应跑本例。

---

### 激活 FPU — CPACR

协处理器访问控制寄存器 **CPACR**（`0xE000ED88` 附近，系统控制空间）：

```
开启 CP10、CP11 的 Full Access
        ↓
否则 VFP 指令触发 UsageFault / Undefined
```

**Ch2 回顾：** 写系统寄存器需 **特权 Handler/Thread privileged**。

**Ch9 详述：** FPSCR、浮点异常、舍入模式。

---

### 单精度与指令

| 指令 | 作用 |
|------|------|
| **`VMOV.F32 Sd, #imm`** | 浮点立即数/寄存器移动 |
| **`VADD.F32 Sd, Sn, Sm`** | 单精度加法 |

**格式：** IEEE 754 **32 bit** — [Ch1 §1.5](../../chapter-01-overview-computing-systems/notes/section-1-5-representation.md) 已建直觉。

---

### 与嵌入式 Linux 路径

| MCU 本书 (M4F) | 应用处理器 (Cortex-A + Linux) |
|----------------|-------------------------------|
| 手写 `VADD.F` | 浮点多在 **C/NEON**；内核 `kernel_fpu_begin()` |
| CPACR 使能 | 用户态 **硬件 FP lazy context** |

飞控姿态：**[24 章](../../../24-Motion-Control-Motor/)** 用 C + libm/NEON 更常见；本程序建立「FPU 存在且需使能」直觉即可。

---

### 可复述要点

1. **M4F 用前必须配置 CPACR** 打开 CP10/11。  
2. **浮点寄存器 s0–s31** 与 **r0–r12 分离**。  
3. 主线可 **跳过 §3.6–3.7**，不影响 Load/Store/中断学习。
