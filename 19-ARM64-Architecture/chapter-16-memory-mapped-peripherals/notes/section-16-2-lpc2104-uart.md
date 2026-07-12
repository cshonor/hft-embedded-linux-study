## §16.2 LPC2104 — UART 通信

> **Ch 16 · 内存映射外设** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### LPC2104 总线架构

| 总线 | 连接 | 速度 |
|------|------|------|
| **AHB** | 内核 · Flash · SRAM | 高 |
| **VPB** | UART · Timer · GPIO 等外设 | 较慢（分频） |

外设寄存器挂在 **VPB** — 访问仍用 **`LDR/STR`**，只是 **等待周期** 可能更长。

**UART0 映射：** 约 **`0xE000C000` – `0xE000C01C`** — 每寄存器 **32 bit 对齐** 偏移。

---

### 关键寄存器（概念）

| 寄存器 | 作用 |
|--------|------|
| **PINSEL0** | 引脚 **复用** — P0.0/P0.1 → **Tx0/Rx0** |
| **U0LCR** | 线控制 — 字长、停止位、校验 |
| **U0DLL/DLM** | 波特率分频 |
| **U0LSR** | **线状态** — Tx 空、Rx 就绪等 |
| **U0THR** | **发送 holding** — 写字节发出 |
| **U0RBR** | 接收缓冲（读） |

具体位定义查 **NXP LPC210x User Manual** — 口述抓 **流程** 即可。

---

### 引脚复用：读-改-写 (R-M-W)

P0.0/P0.1 默认可能是 GPIO，须改 **PINSEL0** 对应 **2 bit 域**：

```asm
    LDR     r1, =PINSEL0_BASE
    LDR     r0, [r1]
    BIC     r0, r0, #MASK_P0_0_P0_1    ; 清旧功能
    ORR     r0, r0, #VAL_UART0_TX_RX
    STR     r0, [r1]
```

**与 Ch5 联动：** 单 bit 字段修改 **不能** 盲目 `STR` 整字 — 会 **破坏其它引脚配置**。

---

### UART 初始化顺序（口述）

```
1. PINSEL → UART 引脚
2. U0LCR：设 DLAB 访问分频 → 写 DLL/DLM → 清 DLAB
3. U0LCR：8N1 等格式
4. （可选）使能 FIFO
```

---

### 发送子程序 — 轮询 LSR

```asm
; r0 = 待发送字节
UART0_SendByte
    STMFD   sp!, {r4, lr}           ; Ch13 序言
    LDR     r4, =U0_BASE
wait_tx:
    LDR     r1, [r4, #U0LSR_off]
    TST     r1, #LSR_THRE            ; Transmitter Holding Register Empty
    BEQ     wait_tx
    STRB    r0, [r4, #U0THR_off]
    LDMFD   sp!, {r4, pc}
```

| 要点 | 说明 |
|------|------|
| **轮询 THRE** | 缓冲区空才 **STRB** 下一字节 |
| **`STRB`** | 只写 **低 8 bit** 到 THR |
| **`BL UART0_SendByte`** | 字符串 = 循环 `BL` |

**升级路径：** [Ch15](../chapter-15-exception-handling-v7m/) — **UART RX/TX 中断** 代替死等 THRE。

---

### 与 Linux / U-Boot

| 环境 | 对应 |
|------|------|
| **U-Boot** | **`serial_init`** · **`putc`** — 同 LSR 轮询或 FIFO |
| **Linux 驱动** | **`8250/`** 核心 · **`writel(c, port+UART_TX)`** |
| **调试** | 第一块 **「能打印」** 的硬件 |

→ [20 构建](../../20-UBoot-Kernel-Build/) early console

---

### 可复述要点

1. **UART = MMIO 寄存器** @ `0xE000C000` 区。  
2. **先 PINSEL，再 LCR/波特率，再发字节**。  
3. **发前读 LSR** — 经典 **轮询驱动** 模板。
