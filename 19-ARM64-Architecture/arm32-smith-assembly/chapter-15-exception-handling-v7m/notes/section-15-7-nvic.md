## §15.7 中断 — 基于 NVIC 的外部中断

> **Ch 15 · 异常处理：v7-M** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### NVIC 是什么

**Nested Vectored Interrupt Controller** — **片上**（非板级 VIC），ARM 定义、各厂实现：

| 能力 | 说明 |
|------|------|
| **使能/禁用** 每路 IRQ | `NVIC_ISER` / `NVIC_ICER` |
| **挂起/清除** | `NVIC_ISPR` / `NVIC_ICPR` |
| **优先级** | 每 IRQ **可编程**（实现 8 bit 里若干有效位） |
| **嵌套** | 高优先级 ISR **可打断** 低优先级 ISR |

**规模：** 规范最多 **496** 外中断；M4 常见 **240** 以内；具体芯片如 **TI Tiva ~65**。

**对比 Ch14 VIC：** NVIC **标准 + CMSIS API**；VIC 为 **SoC 外置 IP**。

---

### 配置外部中断的典型步骤

```
1. 开外设时钟（RCGC/GPIO 等 — 芯片相关）
2. 配外设（如 Timer：load、mode、enable）
3. 外设侧：使能 **中断输出**（如 TIMER_IMR）
4. NVIC：设 **优先级** · **使能 IRQ 号**
5. 全局：清 **PRIMASK** 或设 **BASEPRI**（允许中断）
6. 写 **ISR**（C 函数名 = 向量表槽位 / startup 弱符号覆盖）
```

---

### 书中实例：Timer 0A（Tiva）

**流程口述：**

| 步骤 | 动作 |
|------|------|
| **时钟** | 写 **RCGC** 寄存器打开 **Timer0** 模块时钟 |
| **Timer 配置** | **16 bit 分割** · **向下计数** · **周期模式** · 设 **LOAD** |
| **中断源** | **Timeout** 中断使能 · **Timer enable** |
| **NVIC** | 查手册 **TIMER0A_IRQn** 编号 → **优先级** → **ISER 位置位** |
| **Handler** | 读 **RIS/MIS** 确认 · 写 **ICR** 清标志 · 应用逻辑（翻转 LED 等） |

**与 [Ch16](../chapter-16-memory-mapped-peripherals/) 关系：** §15.7 偏 **中断链路**；Ch16 偏 **寄存器/MMIO 读写** — 合起来完成 **裸机定时器**。

---

### ISR 编写要点

```c
void Timer0A_IRQHandler(void) {
    // 1. 确认/清除外设中断（必须，否则 storm）
    TIMER0_ICR_R = TIMER_ICR_TATOCINT;
    // 2. 短逻辑
    GPIO_PORTF_DATA_R ^= GPIO_PIN_3;
}
// 3. 函数返回 → 编译器 BX lr (EXC_RETURN) → 硬件出栈
```

| 原则 | 原因 |
|------|------|
| **先清中断** | 否则 **重复进入** |
| **保持短小** | 嵌套/抖动 · 实时性 |
| **少阻塞** | 勿在 ISR 里 **长 delay** |

---

### 优先级与嵌套

```
低优先级 Timer ISR 运行中
        ↓ 高优先级 UART IRQ 到来
UART ISR 抢占 · 完成后回到 Timer ISR
```

**BASEPRI** 可屏蔽 **低于某阈值** 的中断 — 临界区优化。

---

### CMSIS 命名

```c
NVIC_EnableIRQ(TIMER0A_IRQn);
NVIC_SetPriority(TIMER0A_IRQn, 2);
```

Startup 文件 **弱定义** `Default_Handler` — 未实现则 **死循环**，链接 **强符号** `Timer0A_IRQHandler` 覆盖向量表项。

---

### 可复述要点

1. **NVIC** = M 系中断 **标准中枢** — 替代 ARM7 **VIC**。  
2. 配中断 = **外设 + NVIC + 全局开中断** 三步缺一不可。  
3. 书中 **Timer0A** 是 **Ch16 MMIO** 的中断延伸模板。
