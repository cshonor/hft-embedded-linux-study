## §15.4 堆栈指针 — MSP/PSP

> **Ch 15 · 异常处理：v7-M** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 两个 SP，一个 r13

Cortex-M **物理上两个栈指针**，软件通过 **`CONTROL.SPSEL`** 与当前模式选择可见的 **SP**：

| 指针 | 全称 | 典型用途 |
|------|------|----------|
| **MSP** | Main Stack Pointer | **Reset 默认** · **Handler 强制** · 内核/裸机 `main` |
| **PSP** | Process Stack Pointer | **RTOS 各任务** 私有栈 |

```
裸机小项目：全程 MSP 即可
RTOS：     内核用 MSP，每个任务 Thread 用 PSP
```

---

### 谁必须用 MSP

| 上下文 | 使用的栈 |
|--------|----------|
| **Handler mode**（任意 ISR/Fault） | **MSP**（硬件规定） |
| **Thread + CONTROL.SPSEL=0** | MSP |
| **Thread + CONTROL.SPSEL=1** | PSP |

**异常入口：** 硬件向 **当前 Thread 所选栈**（MSP 或 PSP）**压栈帧**；Handler 内运行用 **MSP** 取指访问栈 — 细节见 [§15.5](./section-15-5-stack-frames.md) **EXC_RETURN** 记录 **压栈用的是哪条栈**。

---

### 与 Ch13 堆栈指令

**PUSH/POP** 仍适用 — 但 ISR 里若只改 **r4–r11**，在 **硬件已压 r0–r3,r12,LR,PC,xPSR** 之上再压 **callee-save**（AAPCS）。

**C 编译的 ISR：**

```c
void Timer0A_IRQHandler(void) {
    // 编译器自动生成 prologue 保存 r4-r11 等
    TIMER0_ICR_R = …;  // 清中断
}
```

---

### RTOS 任务切换（预告）

**PendSV**（最低优先级异常）中：

```
保存旧任务 PSP 栈上下文
加载新任务 PSP
EXC_RETURN 回到新任务 Thread
```

**口述：** **PSP = 多任务隔离**；**PendSV = 专用「换栈」异常**。

---

### 可复述要点

1. **MSP** = 主栈；**PSP** = 进程/任务栈 — 都在 **r13** 语义下切换。  
2. **Handler 永远特权 + 用 MSP 执行 handler 代码**；压栈目标随 Thread 配置变。  
3. 向量 **0x0** 给出 **MSP 初值** — 复位后第一条栈操作合法。
