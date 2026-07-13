## §2.4 Cortex-M4 — 处理器模式 · 寄存器 · 向量表

> **Ch 2 · 程序员模型** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)  
> **英文：** Cortex-M4 Programmer's Model · → [Ch15 异常详解](../../chapter-15-exception-handling-cortex-m4/)

---

### Cortex-M4 是什么（定位）

**Cortex-M4** = ARM **32 位嵌入式内核**（**ARMv7-M**），主打 **控制 + 适度 DSP**。比 M3 多 **更强 DSP 指令**；**FPU 可选**（带则为 **M4F**）。常见于手表、工控、汽车电子；**STM32F4** 多用此内核。

> **M4 ≠ ARM7 换皮**：模型从 **7 模式 Bank** → **Thread/Handler + 硬件压栈**（下文 + Ch15）。

---

## 一、运行模型：2 种模式 + 2 级特权

### 1. 两种运行模式

| 模式 | 说明 |
|------|------|
| **Thread Mode** | 普通应用 / 用户任务正常执行；**上电默认**进入 |
| **Handler Mode** | **任意中断/异常** 自动进入；**固定特权**；异常返回自动回 Thread |

### 2. 两级访问特权

| 级别 | 能力 |
|------|------|
| **Privileged** | 可操作 **NVIC、MPU、CONTROL、系统控制块** 等；内核与异常处理默认特权 |
| **Unprivileged (User)** | 访问内核寄存器受限 — **RTOS 隔离**业务任务，防篡改内核配置 |

**组合关系（口述）：**

```
Handler Mode  →  永远 Privileged
Thread Mode   →  Privileged 或 Unprivileged（由 CONTROL 决定）
```

上电复位后典型：**Thread + Privileged + MSP**；跑 RTOS 时再切 **Thread Unprivileged + PSP**。

### 3. CONTROL 寄存器（→ Ch15）

- Thread 用 **MSP** 还是 **PSP**
- Thread 是 **特权 / 非特权**
- 专为小型 RTOS / 轻量 OS 提供任务隔离接口

### 对比 ARM7 核心优势

| ARM7 | Cortex-M4 |
|------|-----------|
| 7 种模式 + 多组 banked 寄存器 | **Thread / Handler** + 特权位 |
| ISR 常手动压 r0–r12 | **硬件自动压栈**（8-word 帧，Ch15） |
| 向量表存 `B` 指令 | 向量表存 **函数地址** |

→ 代码更省、**中断延迟更可预测**。

---

## 二、内核寄存器架构

### 1. 基础 16 个内核寄存器

| 寄存器 | 功能 |
|--------|------|
| **R0–R12** | 通用；传参惯例 → [Ch13](../../chapter-13-subroutines-and-stacks/)（APCS / AAPCS） |
| **R13 (SP)** | **双堆栈**：**MSP**（主栈）· **PSP**（进程/任务栈） |
| **R14 (LR)** | 普通调用 = 返回地址；**异常上下文 = EXC_RETURN** 特殊码（非普通 PC） |
| **R15 (PC)** | 程序计数器；**仅 Thumb-2** |

### 双堆栈分工（RTOS 基础）

| 指针 | 谁用 |
|------|------|
| **MSP** | 内核初始化、**异常/中断**、OS 内核代码 |
| **PSP** | 普通用户任务独立栈；**切任务主要换 PSP**，少碰内核栈 |

```
异常入口：硬件用当前 SP（常 MSP）自动压帧
任务切换：换 PSP → 下一任务栈恢复
```

### 2. Cortex-M4F 浮点扩展（Ch9–11，可跳过）

| | |
|--|--|
| **S0–S31** | 32 个单精度浮点寄存器 |
| **FPSCR** | 浮点运算状态 / 异常控制 |

带 FPU 时异常入口还可能自动保存部分 **s** 寄存器（lazy stacking，Ch15）。

---

## 三、xPSR — 三合一状态寄存器

逻辑三段，硬件合为 **xPSR**，一次读可得三类状态：

| 视图 | 全称 | 内容 |
|------|------|------|
| **APSR** | Application | **N Z C V**、饱和标志等 |
| **IPSR** | Interrupt | **当前异常编号**（Thread 时为 **0**；Handler 里为对应号） |
| **EPSR** | Execution | Thumb 相关位 — **M 系只跑 Thumb-2** |

---

## 四、异常向量表（与 ARM7 最核心差异）

### 1. ARM7 ↔ M4

| 特性 | ARM7TDMI | Cortex-M4 |
|------|----------|-----------|
| 表项内容 | 跳转指令 `B handler` | 处理函数 **地址（字）** |
| 表头首项 | Reset 跳转 | **初始 MSP** + Reset_Handler 地址 |
| 风格 | 经典 ARM | 类似 **8051** 向量表 |

### 2. Thumb 地址强制规则

- 向量中函数地址 **LSB 必须为 1**（标记 Thumb）
- 硬件取址时 **自动清 LSB**，得到真实对齐地址

### 3. 表头布局（偏移 · 字节）

| 偏移 | 内容 |
|------|------|
| **0x00** | 上电初始 **MSP**（栈顶） |
| **0x04** | **Reset_Handler** 地址（LSB=1） |
| **0x08+** | NMI · HardFault · … · SVCall · PendSV · SysTick · 外设 IRQ… |

---

## 五、ARM7TDMI vs Cortex-M4 总表

| 维度 | ARM7TDMI | Cortex-M4 |
|------|----------|-----------|
| 运行模式 | 7 模式 + bank | Thread + Handler + 特权位 |
| SP | 各模式独立 SP | **MSP / PSP** 全局双栈 |
| 向量表 | 跳转指令 | **函数指针**；首项 = 初始栈 |
| 指令集 | ARM 32 + Thumb 16 | **仅 Thumb-2**（16/32 混合） |
| 对齐 | 强制严格对齐 | 可配置允许非对齐访问 |

---

## 六、必背考点（口述极简）

1. **Thread / Handler** + **Priv / User**；CONTROL + MPU 做隔离。  
2. **MSP / PSP** 分内核与任务栈 — RTOS 调度基础。  
3. **向量表存地址**（非 `B`）；首字 = **初始 MSP**；handler 地址 **LSB=1**。  
4. 本书实验主平台 **Cortex-M4**；Ch3 起代码按 **Thumb-2** 读。

---

### 与 ARM7 Bank 对照一句

> ARM7：**Bank 换 SP/LR/SPSR**，共用 GPR 还要软件压栈。  
> M4：**硬件自动压 8-word 帧**到当前栈，Handler 统一入口 — 实现不同，都是为了 **少手动画上下文**。
