## Ch16 完整概述 · 内存映射外设

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Memory-Mapped Peripherals · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **MMIO** | 外设 = **固定地址的寄存器** — 用 **`LDR`/`STR`** 控制硬件 |
| **Bring-up 顺序** | **引脚复用 → 外设配置 → 轮询/中断** |
| **三平台三外设** | LPC2104 **UART** · LPC2132 **DAC+sin表** · Tiva **GPIO+时钟** |
| **综合前章** | Ch5 寻址 · Ch7 Q · Ch12 查表 · Ch13 BL/AAPCS · Ch15 NVIC（Timer 可接） |

**前置：** [Ch5 Load/Store](../chapter-05-loads-stores-addressing/) · [Ch13 子程序](../chapter-13-subroutines-stacks/) · [Ch15 异常/NVIC](../chapter-15-exception-handling-v7m/)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **MMIO 总论** | §16.1 | [section-16-1-intro.md](./section-16-1-intro.md) |
| **LPC2104 UART** | §16.2 | [section-16-2-lpc2104-uart.md](./section-16-2-lpc2104-uart.md) |
| **LPC2132 DAC 正弦波** | §16.3 | [section-16-3-lpc2132-dac.md](./section-16-3-lpc2132-dac.md) |
| **Tiva GPIO / 地址掩码** | §16.4 | [section-16-4-tiva-gpio.md](./section-16-4-tiva-gpio.md) |
| **练习** | §16.5 | [section-16-5-exercises.md](./section-16-5-exercises.md) |

---

### 三、知识流（口述版）

```
无专用 I/O 指令 → 外设映射到内存地址空间
        ↓
PINSEL / 时钟(RCGC) → 引脚与外设 alive
        ↓
配 LCR、波特率 / DACR / GPIO DIR
        ↓
轮询 LSR 或 STR 数据寄存器（或 Ch15 中断）
        ↓
Linux：同 MMIO → ioremap · 驱动 readl/writel
```

---

### 四、三节对照

| 平台 | 总线/架构 | 外设 | 特色步骤 |
|------|-----------|------|----------|
| **LPC2104** | ARM7 · AHB/VPB | UART0 @ `0xE000C000` | **PINSEL** · **LSR 轮询** |
| **LPC2132** | ARM7 | DAC @ `0xE006C000` | **Ch12 sin 表** · Q31→10bit |
| **Tiva M4** | v7-M | GPIO Port F | **RCGCGPIO** · **地址掩码写 DATA** |

---

### 五、与 HFT / 嵌入式链

| 模块 | 关联 |
|------|------|
| [Ch5 §5.6 位带](../chapter-05-loads-stores-addressing/notes/section-5-6-bit-banded.md) | Tiva **地址掩码** 同类「单 bit 安全写」 |
| [Ch12 查表](../chapter-12-tables/) | DAC 正弦 **LUT + 缩放** |
| [21 驱动](../../21-Linux-Device-Driver/) | **`readl`/`writel`** · **`platform_device`** |
| [22 DT](../../22-Device-Tree-Study/) | 寄存器基址进 **设备树** |
| [08 MikanOS GOP](../../08-system-low-level-hands-on/) | 帧缓冲也是 **MMIO 写像素** |
| [20 U-Boot](../../20-UBoot-Kernel-Build/) | 板级 **early UART** 打印 |

---

### 六、下一章

→ **[Ch17 ARM/Thumb/Thumb-2](../chapter-17-arm-thumb-thumb2-instructions/)**（选读）或继续精读链 **[Ch18 C/Asm](../chapter-18-mixing-c-and-assembly/)**

---

### 七、精读验收

能 **不看书记** 说出：MMIO 为何用 LDR/STR、UART 发送前 **轮询 LSR**、DAC **10bit 缩放公式**、Tiva **开时钟再碰 GPIO**。
