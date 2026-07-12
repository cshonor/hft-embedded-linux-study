## §15.2 操作模式与特权级别

> **Ch 15 · 异常处理：v7-M** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 两种运行模式

| 模式 | 何时 | 说明 |
|------|------|------|
| **Thread mode** | 正常应用代码 | 复位后默认；跑 `main`、任务 |
| **Handler mode** | 任意异常/中断 | **自动进入**；跑 ISR、Fault handler |

**口述：** 没有 ARM7 的 irq/fiq/abt/und/svc **七种模式 bank** — 只有 **Thread vs Handler** 这一条轴。

```
Thread 执行
    ↓ 异常
Handler 执行 ISR
    ↓ EXC_RETURN
Thread 恢复
```

---

### 两种特权级别

| 级别 | 能力 |
|------|------|
| **Privileged（特权）** | 访问 **NVIC、MPU、CONTROL、系统控制块** |
| **Unprivileged（用户）** | **不能** 直接改 NVIC/MPU — 须 **SVC** 进内核 |

**规则：**

- **Handler mode 永远是 Privileged**  
- **Thread mode** 可为 Privileged 或 Unprivileged — 由 **`CONTROL.nPRIV`** 决定  

---

### CONTROL 寄存器（概念）

| 位 | 作用 |
|----|------|
| **nPRIV** | Thread 是否 **非特权** |
| **SPSEL** | Thread 用 **MSP 还是 PSP**（Handler **强制 MSP**） |

**RTOS 用法：**

```
内核 Thread（Privileged）+ MSP
用户任务 Thread（Unprivileged）+ PSP
PendSV Handler 切换 PSP → 换任务栈
```

→ [§15.4](./section-15-4-stack-pointers.md) · [§15.6 SVCall/PendSV](./section-15-6-fault-types.md)

---

### 与 ARM7 对比

| ARM7 (Ch14) | v7-M |
|-------------|------|
| User / IRQ / FIQ / SVC / Abort / Und / System | **Thread + Handler** |
| 换模式 = 换 **banked SP/LR** | 换模式 + **硬件压栈** |
| 特权 = 模式本身 | **Handler 特权 + Thread 可选 nPRIV** |

**Linux 粗对照：** Thread 用户态 ≈ **EL0**；Handler 内核态处理 trap ≈ **EL1**（AArch64 见 [奔跑吧](../arm64-programming-practice/)）。

---

### 可复述要点

1. **异常 ⇒ Handler + 特权**；应用 ⇒ **Thread**。  
2. **CONTROL** 实现 **用户/内核隔离** 与 **双栈选择**。  
3. 简化的模式是为 **低延迟 + RTOS** 服务，不是功能缩水。
