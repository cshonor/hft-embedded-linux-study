## §15.3 向量表

> **Ch 15 · 异常处理：v7-M** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 与 ARM7 的根本差异

| | **ARM7 (Ch14)** | **Cortex-M (本章)** |
|---|-----------------|---------------------|
| 槽内容 | **`B Handler`** 指令 | **32 bit Handler 地址** |
| **0x00** | Reset **跳转** | **初始 MSP 值**（非 PC！） |
| **0x04** | Undefined | **Reset Handler 地址** |

**复位序列：**

```
1. 从 0x0 加载值 → 写入 MSP（栈顶建立）
2. 从 0x4 加载值 → 写入 PC（开始执行 Reset_Handler）
```

→ [20 U-Boot](../../20-UBoot-Kernel-Build/) / 启动代码 **`__Vectors`** 数组同结构（CMSIS）。

---

### 系统异常槽（前几项）

| 偏移 | 异常 |
|------|------|
| **0x00** | 初始 **MSP** |
| **0x04** | Reset |
| **0x08** | NMI |
| **0x0C** | HardFault |
| **0x10** | MemManage |
| **0x14** | BusFault |
| **0x18** | UsageFault |
| **0x2C** | SVCall |
| **0x38** | PendSV |
| **0x3C** | SysTick |
| **0x40+** | **外部 IRQ #0, #1, …** |

具体 **IRQ 编号** 与 **外设** 对应关系查 **芯片数据手册**（如 Tiva **Timer0A** 占某 IRQ 号）。

---

### 向量表重定位

**VTOR** 寄存器可将向量表移到 **Flash/RAM** 任意对齐地址 — Bootloader 升级、RAM 调试常用。

---

### 链接脚本中的向量

```c
// CMSIS 风格（概念）
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))((uint32_t)&__StackTop),  // 0x0 MSP
    Reset_Handler,                             // 0x4
    NMI_Handler,
    HardFault_Handler,
    // …
};
```

Startup **汇编** 往往只设 **栈 + 调 SystemInit + 跳 main**。

---

### 可复述要点

1. **0x0 = MSP，0x4 = Reset** — 与 ARM7 **完全不同**，勿混淆。  
2. 向量槽 = **函数指针**，C 里可直接 **填函数名**。  
3. 外设中断从 **0x40** 起按 **IRQ 编号** 排列。
