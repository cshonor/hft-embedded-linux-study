## §16.4 Tiva Launchpad — GPIO 操作

> **Ch 16 · 内存映射外设** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 平台：TM4C123GH6PM (Cortex-M4)

**Tiva Launchpad** — 本书 **v7-M** MMIO 主示例：

| 硬件 | 连接 |
|------|------|
| **三色 LED** | **Port F**：**PF1 红 · PF2 绿 · PF3 蓝**（低电平点亮，查板卡原理图） |
| **架构** | Cortex-M4 — [Ch15](../chapter-15-exception-handling-v7m/) 异常/NVIC 同族 |

---

### Tiva 与外设时钟（与 LPC 差异）

访问 **任何** 外设寄存器前，须 **开时钟门控**：

| 步骤 | 寄存器（概念） |
|------|----------------|
| **系统时钟** | **RCC** — 确认主频/PLL（startup 常已配） |
| **GPIO 时钟** | **`RCGCGPIO`** — 对 **Port F** 置 **对应 bit** |

```c
// 概念：RCGCGPIO_R |= (1 << 5);  // Port F
```

**口述：** **未开时钟 = 读写的寄存器无响应** — M4 常见 **HardFault 或静默失败**。

---

### GPIO 配置流程

```
1. RCGCGPIO → 使能 Port F 时钟
2. GPIO_DIR → PF1/PF2/PF3 设为输出
3. （可选）GPIO_AFSEL = 0 → 普通 GPIO
4. GPIO_DEN → 数字使能
5. 写 DATA / 掩码写 → 点亮 LED
```

具体寄存器名以 **Tiva Data Sheet** 为准（`GPIO_PORTF_*`）。

---

### 三色 LED 循环

**逻辑：** 每次只亮 **一种颜色** — 先 **灭全部**，再 **亮某一脚**。

传统写法 **R-M-W** `DATA`：

```asm
    LDR     r0, [r1, #GPIO_DATA_off]
    BIC     r0, r0, #LED_ALL
    ORR     r0, r0, #LED_RED
    STR     r0, [r1, #GPIO_DATA_off]
```

**风险：** 中断/并发下 **读-改-写** 可能 **打丢其它位** — [Ch5 §5.6](../chapter-05-loads-stores-addressing/notes/section-5-6-bit-banded.md) **位带** 是为解决此类问题。

---

### 地址掩码 (Address Masking)

Tiva **GPIO DATA** 映射：**基址 + 偏移** 的 **地址位 [9:2]** 作 **写掩码** — 只更新 **掩码为 1** 的 **PF[7:0]** 对应位。

```
写 (GPIO_BASE + (pin_mask << 2)) = data_byte
        ↓
硬件只修改 mask 选中的引脚，其余位不变
```

| 对比 | 机制 |
|------|------|
| **Ch5 位带** | 1 bit → 独立 **32 bit 别名地址** |
| **Tiva 地址掩码** | **地址偏移** 编码 **哪些 pin 参与写** |

**效果：** **单条 STR** 安全改 **PF1** 而不误动 **PF2/PF3** — 无需完整 R-M-W。

**示例（概念）：**

```asm
; 只动 PF1：mask bit1 → 偏移 (1<<2) 或手册规定编码
    LDR     r2, =GPIO_PORTF_BASE + GPIO_PIN_1_MASKED_OFFSET
    MOV     r0, #0                    ; 或点亮值
    STR     r0, [r2]
```

（精确偏移公式以 **TM4C123 GPIO chapter** 为准 — 口述记 **「地址带掩码」** 即可。）

---

### 与 Ch15 联调

LED **闪烁** 可用 **软件延时循环**；稳定周期用 **Timer0A IRQ**（[§15.7](../chapter-15-exception-handling-v7m/notes/section-15-7-nvic.md)）在 ISR 里 **切换掩码写**。

---

### 可复述要点

1. **Tiva：先 RCGCGPIO，再 DIR/DEN，再 DATA**。  
2. Launchpad **PF1/2/3 = RGB**。  
3. **地址掩码** ≈ 位带思想 — **单 STR 安全改单 pin**。
