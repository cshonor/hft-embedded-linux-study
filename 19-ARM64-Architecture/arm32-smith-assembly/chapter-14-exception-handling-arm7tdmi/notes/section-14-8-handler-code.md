## §14.8 处理异常的程序 — 复位 · 未定义 · VIC · 中止 · SVC

> **Ch 14 · 异常处理：ARM7TDMI** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 本节地位

全书 **最长实例节** — 把 §14.4–14.7 落到 **可运行汇编**。按异常类型分块理解即可，不必死记每行。

---

### 1. 复位 (Reset)

**特点：** **不返回** — 系统 **冷启动**。

**典型流程：**

```
Reset_Handler:
    关闭/忽略无关中断
    为各模式设置 SP（IRQ/FIQ/SVC/Abort/Undefined/System）
    初始化时钟、内存控制器、关键 I/O
    B       main              ; 或复制 .data、清零 .bss 后跳 C runtime
```

**与嵌入式链：**

- [20 U-Boot](../../20-UBoot-Kernel-Build/) **`start.S`** — 设栈、清 BSS、跳 C  
- [08 MikanOS Loader](../../08-system-low-level-hands-on/) — UEFI 入口同类 **分阶段 init**

---

### 2. 未定义指令 (Undefined)

**书中示例逻辑：**

```
1. Undefined_Handler 压栈
2. 读触发异常的指令字（LR 附近）
3. 解码：是否为「软件仿真浮点/协处理器」 opcode
4. 若是 → 用整数库完成运算，更新寄存器
5. 调整 PC 跳过该指令
6. SUBS 返回
```

**价值：** 展示 **异常作扩展机制** — 不是只能 `panic`。

---

### 3. 中断 + VIC (Vectored Interrupt Controller)

**问题：**  dozens 外设共享 **IRQ** — 公共 handler **轮询** 谁 pending → **O(n)** 慢。

**VIC 硬件：**

| 功能 | 说明 |
|------|------|
| **优先级/屏蔽** | 每源可配 |
| **向量地址** | 当前最高 pending 源 → **专用 ISR 地址** |
| **CPU 接口** | 向量表槽：`LDR pc, [vic_base + offset]` |

**效果：** PC **一条指令** 跳进 **UART_ISR** / **Timer_ISR** — 无需软件查表循环。

**对照：**

| ARM7 + VIC | Cortex-M |
|------------|----------|
| 板级 **VIC IP** | 片上 **NVIC**（Ch15） |
| 灵活但非标准 | **CMSIS** 统一 API |

→ [Ch16](../chapter-16-memory-mapped-peripherals/) UART 中断清 **RI/TI** 位在 **专用 ISR** 末尾。

---

### 4. 中止 (Aborts) — MMU 环境

**Prefetch / Data Abort handler 可能：**

```
1. 读 fault 状态寄存器（FSR/FAR — 平台相关）
2. 判因：对齐 / 权限 / 缺页
3. 若可修复：更新页表、TLB invalidate
4. SUBS pc, lr, #4 或 #8  重试 fault 指令
5. 若不可修复：转 C 信号/内核 oops
```

**LR 修正：**

| 类型 | 典型 `SUBS pc, lr, #imm` |
|------|--------------------------|
| **Prefetch Abort** | **#4** |
| **Data Abort** | **#8** |

Linux **用户态缺页** — 内核 fault handler 修页后 **返回用户继续** — 同 **重试** 思想（[04 LKD](../../04-Linux-Kernel-Development/)）。

---

### 5. 系统调用 (SVC)

**触发：** 用户代码执行 **`SVC #n`**。

**Handler 要做：**

```
1. 从 LR 算出现 faulting 指令地址
2. LDR 该指令字，提取 immediate n（服务号）
3. switch(n): 控制台 I/O、exit、semihosting…
4. 结果写 r0
5. SUBS pc, lr, #4  返回用户
```

**Semihosting：** `SVC` 把 **printf/read** 转给调试器 — **仅开发**；量产须 **移除** 或换 **UART**。

**现代对照：** AArch64 **`SVC #0`** → Linux **syscall**；编号在 **x8/x0**（奔跑吧 / TLPI）。

---

### 实例阅读顺序（建议）

1. **Reset** — 看懂 **多模式 SP**  
2. **IRQ + VIC** — **LDR PC** 向量分发  
3. **SVC** — **解码指令 + 服务表**  
4. **Undefined / Abort** — 需要 MMU/仿真时再深读  

---

### 可复述要点

1. **Reset** = 设栈 + init + **不返回**。  
2. **VIC** = 多 IRQ 源的 **硬件向量** — 避免公共 ISR 轮询。  
3. **Abort** = 可 **修复后重试** 或 **上报 fatal**。  
4. **SVC** = **系统调用/Semihosting** — 从指令流 **取号** 再分派。
