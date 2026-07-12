## §18.4 C 与汇编相互调用 — APCS

> **Ch 18 · C 与汇编混合编程** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 核心前提

**无论 C→asm 还是 asm→C：**

| 规则 | 内容 |
|------|------|
| **AAPCS** | [Ch13 §13.5](../chapter-13-subroutines-stacks/notes/section-13-5-apcs.md) |
| **参数** | **r0–r3** 前四个；更多 **栈**（降序、8 对齐） |
| **返回值** | **r0**（64-bit **`r0+r1`**） |
| **浮点** | **s0–s15** 参/返；**s16–s31** callee-save |

**C 侧声明：**

```c
extern int asm_add(int a, int b);      /* 汇编实现 */
extern int c_helper(int x);            /* C 实现，供 asm 调 */
```

**汇编侧：**

```asm
    IMPORT  c_helper
    EXPORT  asm_add
```

（GNU：**`.global asm_add`** · **`.extern c_helper`**）

---

### 例 1：汇编调 C（`BL c_add`）

```asm
asm_wrapper
    STMFD   sp!, {r4, lr}
    MOV     r0, #1              ; 第 1 参
    MOV     r1, #2              ; 第 2 参
    ; 第 5+ 参才 PUSH — 此处无
    BL      c_add               ; 返回 r0
    ; 结果已在 r0
    LDMFD   sp!, {r4, pc}
```

**第 5 个参数起：** 调用者 **`PUSH`** → **`BL`** → C 用 **`[sp, #offset]`** 读 — [Ch13 §13.4](../chapter-13-subroutines-stacks/notes/section-13-4-parameters.md)。

---

### 例 2：C 调汇编（`strcpy` 类）

**C：**

```c
int main(void) {
    char dst[32];
    asm_strcpy(dst, "hello");    /* r0=dst, r1=src */
    return 0;
}
```

**Asm（embedded 或 .S）：**

```asm
asm_strcpy
    ; r0=dst, r1=src — AAPCS 已就位
    PUSH    {r4, lr}
loop
    LDRB    r2, [r1], #1
    STRB    r2, [r0], #1
    CMP     r2, #0
    BNE     loop
    POP     {pc}                ; 或 POP {r4, lr}; BX lr
```

**`extern "C"`**（C++）避免 name mangling。

---

### 例 3：硬件指令封装 — VCVT 库

[Ch9–11](../chapter-09-floating-point-basics/) **定点 ↔ 浮点**：

```asm
; float fixed_to_float(int32_t q31)  — 参数 r0
    VCVT.F32.S32 s0, s0, #31    ; 概念指令
    BX      lr                  ; 返回 float 在 s0 / r0 依 ABI
```

**C 侧：**

```c
float y = fixed_to_float(x);   /* 像调普通库 */
```

**价值：** **一条 VCVT** 打包成 **可链接 API** — DSP/飞控 **libm 补充**。

---

### Interworking 提醒

ARM7 **ARM 调 Thumb asm**（或反之）→ [Ch17 Veneer](../chapter-17-arm-thumb-thumb2-instructions/notes/section-17-5-interworking.md)。  
**M4 全 Thumb** — **`BL asm_strcpy`** 即可。

---

### 与内核 / U-Boot

| 项目 | 模式 |
|------|------|
| **U-Boot** | **`start.S`** → **`board_init_f`** C |
| **Linux** | **`head.S`** → **`start_kernel`** |
| **驱动** | 几乎全 **C** + **`readl`**；asm 在 **arch/** 库 |

→ [20 构建](../../20-UBoot-Kernel-Build/) · [21 驱动](../../21-Linux-Device-Driver/)

---

### 可复述要点

1. **互调 = 同一 AAPCS** — 方向无关。  
2. **C 调 asm**：`extern` + **r0–r3**；**asm 调 C**：**`BL`** + 设 **r0–r3**。  
3. **Embedded/asm 库** = 把 **VCVT/MMIO/拷贝** 封成 **C 可调函数**。
