## Ch15 完整概述 · 异常处理：v7-M

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Exception Handling: v7-M · **选读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **简化模式** | **Thread / Handler** + **特权 / 用户** — 替代 ARM7 七种模式 |
| **向量表** | **0x0 = 初始 MSP**；**0x4 起 = handler 地址**（非 `B` 指令） |
| **双栈** | **MSP**（内核/异常）· **PSP**（任务/用户） |
| **硬件栈帧** | 自动压 **r0–r3,r12,LR,PC,xPSR**（+ 可选 FPU） |
| **EXC_RETURN** | **LR** 中的 magic 值 — **`BX lr`** 触发硬件出栈 |
| **NVIC** | 片上 **中断使能/优先级/嵌套** — 替代板级 VIC |

**前置：** [Ch2 Cortex-M4](../chapter-02-programmers-model/notes/section-2-4-cortex-m4.md) · [Ch13 堆栈/AAPCS](../chapter-13-subroutines-stacks/)  
**对照：** [Ch14 ARM7](../chapter-14-exception-handling-arm7tdmi/notes/section-0-本章完整概述.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **动机 · vs ARM7** | §15.1 | [section-15-1-intro.md](./section-15-1-intro.md) |
| **Thread/Handler · 特权** | §15.2 | [section-15-2-modes-privilege.md](./section-15-2-modes-privilege.md) |
| **向量表** | §15.3 | [section-15-3-vector-table.md](./section-15-3-vector-table.md) |
| **MSP/PSP** | §15.4 | [section-15-4-stack-pointers.md](./section-15-4-stack-pointers.md) |
| **硬件出入栈 · EXC_RETURN** | §15.5 | [section-15-5-stack-frames.md](./section-15-5-stack-frames.md) |
| **Fault · SVCall · SysTick** | §15.6 | [section-15-6-fault-types.md](./section-15-6-fault-types.md) |
| **NVIC · Timer 实例** | §15.7 | [section-15-7-nvic.md](./section-15-7-nvic.md) |
| **练习** | §15.8 | [section-15-8-exercises.md](./section-15-8-exercises.md) |

---

### 三、知识流（口述版）

```
Reset：读 0x0→MSP，0x4→PC
        ↓
Thread 跑应用（MSP 或 PSP）
        ↓
异常/IRQ → Handler 模式 · 硬件压 8 字栈帧 · LR=EXC_RETURN
        ↓
C/Asm ISR 处理（常只需清外设标志）
        ↓
BX lr（EXC_RETURN）→ 硬件弹栈 · 回 Thread/Handler
        ↓
Ch16 MMIO 配外设 · RTOS：PendSV 切换 PSP
```

---

### 四、ARM7 vs v7-M 速查

| | **Ch14 ARM7** | **Ch15 v7-M** |
|---|---------------|---------------|
| 模式 | 7 种 + bank | **2 模式 + 特权位** |
| 向量 | **`B` 指令** | **Handler 地址** |
| 0x0 | Reset 跳转 | **初始 MSP 值** |
| 保存现场 | 软件为主 + SPSR | **硬件 8 寄存器帧** |
| 返回 | **`SUBS pc,lr,#n`** | **`BX lr` (EXC_RETURN)** |
| 中断控制器 | 板级 **VIC** | 片上 **NVIC** |

---

### 五、固定高优先级异常

| 异常 | 优先级（数值越小越高） |
|------|------------------------|
| **Reset** | **-3** |
| **NMI** | **-2** |
| **HardFault** | **-1** |

其余 **可编程**（0–255，实现裁剪位数）。

---

### 六、与 HFT / 嵌入式链

| 模块 | 关联 |
|------|------|
| [Ch16 MMIO](../chapter-16-memory-mapped-peripherals/) | Timer/GPIO **中断使能** 接 NVIC |
| [04 LKD](../../04-Linux-Kernel-Development/) | 内核 irq 框架 · **HardFault ≈ 内核 panic 前兆** |
| [21 驱动](../../21-Linux-Device-Driver/) | top half 短 ISR |
| [奔跑吧 GIC](../arm64-programming-practice/chapter-13-gic-v2/) | 多核 GIC vs 单片机 NVIC |
| [24 飞控](../../24-Motion-Control-Motor/) | **SysTick/PendSV** · RTOS 调度 |

---

### 七、阅读建议

**M4 裸机/RTOS 必读权重 > Ch14。** 抓 **§15.3–15.5（向量/双栈/栈帧）+ §15.7（NVIC 配 Timer）**；Fault 细分可对照 CMSIS 再深读。

---

### 八、下一章

→ **[Ch16 内存映射外设](../chapter-16-memory-mapped-peripherals/)**（**精读** — UART/GPIO 与中断联调）
