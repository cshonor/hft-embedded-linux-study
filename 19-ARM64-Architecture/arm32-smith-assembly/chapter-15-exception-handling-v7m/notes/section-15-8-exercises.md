## §15.8 练习题

> **Ch 15 · 异常处理：v7-M** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 练习方向（原书典型）

| 类型 | 练什么 |
|------|--------|
| **向量表** | 列出 **0x0 MSP**、**0x4 Reset**、**0x40 IRQ0** |
| **栈帧** | 画 **8 word** 硬件压栈顺序 |
| **EXC_RETURN** | 解释 **0xFFFFFFF9** vs **0xFFFFFFFD** |
| **NVIC** | 对 **GPIO/Timer** 写 **Enable + Priority** |
| **Fault** | 故意 **除零/空指针** 进 **HardFault** · 读 **CFSR** |
| **对比** | 同一 ISR：**Ch14 手写 SUBS** vs **Ch15 C 返回** |

---

### 自测 Checklist

- [ ] **Thread vs Handler** · **Privileged vs Unprivileged**  
- [ ] **MSP vs PSP** — Handler 用哪条、Thread 如何选  
- [ ] 硬件压栈 **8 寄存器** 名单  
- [ ] **`BX lr`** 在 ISR 末尾的含义  
- [ ] **Reset/NMI/HardFault** 优先级 **-3/-2/-1**  
- [ ] **SysTick / PendSV / SVCall** 各干什么  
- [ ] NVIC 配置 **四步**：外设 · NVIC · 开总中断 · ISR  

---

### 常见错误

| 错误 | 后果 |
|------|------|
| 向量 **0x0 填 Reset 地址** | **MSP 错乱** → 立即 HardFault |
| ISR **不清 Timer/GPIO 标志** | **中断风暴** |
| 把 **EXC_RETURN** 当普通地址 **`BX`** | 非法跳转 |
| **PRIMASK=1** 忘记开中断 | 外设 **永不进 ISR** |

---

### 延伸

| 方向 | 参考 |
|------|------|
| **Ch16** | Tiva **GPIO/UART** + NVIC |
| **FreeRTOS** | **PendSV + SysTick** 源码 |
| **04 LKD** | **`request_irq`** · **`local_irq_enable`** |
| **奔跑吧 Ch13** | **GIC** 分发（A 核） |

---

### 可复述要点

1. M4 裸机 **中断实验** 以本章 + Ch16 为准。  
2. 闭卷画 **向量表头两项 + 8-word 栈帧 + EXC_RETURN 返回路径**。  
3. **NVIC + 清标志** 是 ISR **最低合格线**。
