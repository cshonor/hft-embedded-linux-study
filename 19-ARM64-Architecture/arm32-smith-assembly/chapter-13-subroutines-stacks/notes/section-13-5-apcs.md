## §13.5 ARM APCS / AAPCS — 应用过程调用标准

> **Ch 13 · 子程序与堆栈** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 为什么需要调用标准

| 场景 | 要求 |
|------|------|
| **C 调 asm** | C 编译器按标准放参数；asm 必须同规则读 |
| **asm 调 C** | `printf` 等期望 **r0 = format** |
| **多编译器 / 静态库** | 目标文件 **可链接** |

**APCS / ATPCS**（旧称）→ **AAPCS**（ARM Application Procedure Call Standard）— 本书 **§13.5** 讲同一 **契约**；AArch64 有 **AAPCS64**（[奔跑吧](../../../arm64-programming-practice/) 用 **x0–x7**）。

**合规汇编例（C 五参、栈上第 5 参、返回 r0）** → [CSAPP · ABI 实战例](../../../../01-CSAPP-3rd/chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md#6-遵守-abi-的实战例子arm32--aarch64--c汇编)

---

### 核心寄存器分工（32-bit ARM）

| 寄存器 | AAPCS 角色 |
|--------|------------|
| **r0–r3** | 参数 + **返回值**；**调用者保存**（callee 可随意改写） |
| **r4–r8, r10, r11** | **被调用者保存** — 子程序若使用，**序言压栈、尾声恢复** |
| **r9** | 平台相关（部分为 **TLS**）；慎作长期临时 |
| **r12** | **IP** — 内部跳转/scratch；调用间可破坏 |
| **r13** | **SP** — 必须 **8 字节对齐** |
| **r14** | **LR** |
| **r15** | **PC** |

**口述：** **r0–r3 随便用**；**r4–r11 用了就要还**。

---

### 浮点（Cortex-M4 / VFP）

| 寄存器 | 角色 |
|--------|------|
| **s0–s15** | 参数 + 返回值 + 临时 — **调用者保存** |
| **s16–s31** | **被调用者保存** — 浮点子程序若用须 **VPUSH/VPOP** 或等价 |

与 [Ch9–11](../chapter-09-floating-point-basics/) 浮点例程一致：**`sin` 在 s0 进、出**。

---

### 栈对齐

**AAPCS 要求 SP 在 **公共接口** 处 **8 字节对齐**（双字）。

| 原因 | |
|------|--|
| **LDRD/STRD**、**64 bit 参数** | 需要 8 对齐 |
| **调用 C 库** | 未对齐可能 **HardFault** 或性能损失 |

**技巧：** 奇数个 **32 bit** 栈参数时，编译器常插 **padding** 保 8 对齐。

---

### 汇编 ↔ C 最小示例

**C 侧：**

```c
int add(int a, int b);
/* 调用：r0=a, r1=b → 返回 r0 */
```

**Asm 侧：**

```asm
    .global add
    .type   add, %function
add:
    ADD     r0, r0, r1      ; 只改 r0-r3，无需保存 r4+
    BX      lr
```

**Asm 调 C：**

```asm
    LDR     r0, =fmt_string
    BL      printf          ; 按 AAPCS 传参；可能破坏 r0-r3,r12
```

完整 **extern、.global、`.type`** → [Ch18 混合编程](../chapter-18-mixing-c-and-assembly/)。

---

### 与 Linux 内核 / U-Boot

| 环境 | 约定 |
|------|------|
| **内核 ARM32** | 内核自有宏 **`ENTRY`/`-save`**，思想同 callee-save |
| **U-Boot** | 板级 **`.S`** 设 **SP**，**`bl board_init_f`** |
| **AArch64 内核** | **x0–x5** 参数；**x19–x28** callee-save |

→ [20 构建](../../20-UBoot-Kernel-Build/) · [04 LKD](../../04-Linux-Kernel-Development/)

---

### 可复述要点

1. **r0–r3**：参 + 返；**r4–r8,r10,r11**：用了必还。  
2. **M4：s16–s31** callee-save；**s0–s15** 随便用。  
3. **SP 8 字节对齐** — 调 C 前检查。  
4. 混 C/asm **不是可选** — 违反 AAPCS = 静默数据损坏。
