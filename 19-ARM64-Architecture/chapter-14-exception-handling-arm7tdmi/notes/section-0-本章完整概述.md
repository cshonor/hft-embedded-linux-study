## Ch14 完整概述 · 异常处理：ARM7TDMI

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Exception Handling: ARM7TDMI · **选读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **异常分类** | **中断**（IRQ/FIQ/SVC）vs **错误**（未定义/预取中止/数据中止） |
| **硬件序列** | **CPSR→SPSR** · 切模式 · **LR←返回址** · **PC←向量** |
| **向量表** | 存 **`B`/`LDR PC`**，不是裸地址；**FIQ 在表末** 可紧接写 handler |
| **软件 handler** | 压栈 r0–r12 · 处理 · **`SUBS pc, lr, #n`** 原子返回 |
| **VIC** | 多外设中断 **硬件向量** — 替代轮询 |

**前置：** [Ch2 七种模式/CPSR](../chapter-02-programmers-model/notes/section-2-3-arm7tdmi.md) · [Ch13 堆栈](../chapter-13-subroutines-stacks/notes/section-0-本章完整概述.md)  
**对照：** **Cortex-M** → [Ch15](../chapter-15-exception-handling-v7m/) · **AArch64** → [奔跑吧 Ch11–13](../arm64-programming-practice/)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **动机 · ARM7 vs M** | §14.1 | [section-14-1-intro.md](./section-14-1-intro.md) |
| **IRQ/FIQ/SVC** | §14.2 | [section-14-2-interrupts.md](./section-14-2-interrupts.md) |
| **未定义/中止** | §14.3 | [section-14-3-error-conditions.md](./section-14-3-error-conditions.md) |
| **硬件异常序列** | §14.4 | [section-14-4-exception-sequence.md](./section-14-4-exception-sequence.md) |
| **向量表** | §14.5 | [section-14-5-vector-table.md](./section-14-5-vector-table.md) |
| **Handler · 优先级** | §14.6 | [section-14-6-handlers-priority.md](./section-14-6-handlers-priority.md) |
| **机制小结** | §14.7 | [section-14-7-mechanism.md](./section-14-7-mechanism.md) |
| **代码实例** | §14.8 | [section-14-8-handler-code.md](./section-14-8-handler-code.md) |
| **练习** | §14.9 | [section-14-9-exercises.md](./section-14-9-exercises.md) |

---

### 三、知识流（口述版）

```
正常取指执行
        ↓
外设/ fault / SVC / Reset
        ↓
硬件：SPSR←CPSR · 切特权模式 · LR←PC_adj · PC←vector
        ↓
软件 handler：PUSH 通用寄存器 · 清中断源 · 服务
        ↓
POP · SUBS pc, lr, #4/#8  → CPSR←SPSR · 返回被断程序
        ↓
Ch15 NVIC/MSP · 04 LKD 中断 · 21 驱动 top half
```

---

### 四、异常优先级（高 → 低）

**Reset > Data Abort > FIQ > IRQ > Prefetch Abort > SVC / Undefined**

同时 pending 时，CPU 按此顺序 **先响应更高优先级**。

---

### 五、ARM7 vs Cortex-M 速查

| | **ARM7TDMI (Ch14)** | **v7-M (Ch15)** |
|---|---------------------|-----------------|
| 模式 | 7 种 **banked SP/LR** | **Handler/Main** + **MSP/PSP** |
| 向量表 | **`B` 指令** @ 0x0 | **Handler 地址** 表 |
| 快中断 | **FIQ** 私有 r8–r12 | **无 FIQ**；NVIC 优先级 |
| 控制器 | **VIC**（板级） | **NVIC**（片上标准） |

---

### 六、与 HFT / 嵌入式链

| 模块 | 关联 |
|------|------|
| [04 LKD](../../04-Linux-Kernel-Development/) | 内核 **trap/irq** 框架 |
| [Ch16 MMIO](../chapter-16-memory-mapped-peripherals/) | 外设 **置位清中断** 在 handler 里 |
| [21 驱动](../../21-Linux-Device-Driver/) | **top/bottom half** |
| [20 U-Boot](../../20-UBoot-Kernel-Build/) | **Reset vector** 链 |
| [奔跑吧 GIC](../arm64-programming-practice/chapter-13-gic-v2/) | 多核中断分发（概念升级） |

---

### 七、阅读建议（选读章）

抓 **§14.4 硬件四步 + §14.5 向量 + §14.7 优先级 + §14.8 VIC/SVC 概念** 即可；细节指令偏移与 MMU 中止可压缩。做 **M4 裸机** 优先 **Ch15**。

---

### 八、下一章

→ **[Ch15 异常处理 v7-M](../chapter-15-exception-handling-v7m/)**（Cortex-M / NVIC — 更贴近常见 MCU）
