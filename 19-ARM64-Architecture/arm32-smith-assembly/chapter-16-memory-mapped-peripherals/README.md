# Ch 16 · 内存映射外设

> ***ARM Assembly Language*** — William Sw Smith · **精读**  
> **English:** Memory-Mapped Peripherals

---

## 本章定位

| | |
|---|---|
| **角色** | **精读** — **MMIO 裸机综合**（UART · DAC · GPIO） |
| **核心模式** | **LDR/STR 寄存器** · **PINSEL/RCGC** · **LSR 轮询** · **查表+缩放** · **地址掩码** |
| **前置** | [Ch5](../chapter-05-loads-stores-addressing/) · [Ch12](../chapter-12-tables/) · [Ch13](../chapter-13-subroutines-stacks/) · [Ch15](../chapter-15-exception-handling-v7m/) |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§16.1** | 简介 | [notes/section-16-1-intro.md](./notes/section-16-1-intro.md) |
| **§16.2** | LPC2104 — UART 通信 | [notes/section-16-2-lpc2104-uart.md](./notes/section-16-2-lpc2104-uart.md) |
| **§16.3** | LPC2132 — D/A 转换器生成正弦波 | [notes/section-16-3-lpc2132-dac.md](./notes/section-16-3-lpc2132-dac.md) |
| **§16.4** | Tiva Launchpad — GPIO 操作 | [notes/section-16-4-tiva-gpio.md](./notes/section-16-4-tiva-gpio.md) |
| **§16.5** | 练习题 | [notes/section-16-5-exercises.md](./notes/section-16-5-exercises.md) |

---

## 本章 Checklist

- [ ] 说清 **MMIO**：外设地址 + **LDR/STR**，无专用 I/O 指令
- [ ] **LPC2104 UART**：PINSEL → LCR/波特率 → **轮询 LSR(THRE)** → **STRB THR**
- [ ] **LPC2132 DAC**：**DACR @ 0xE006C000** · **[15:6]** · **Ch12 sin + `512·sin+512`**
- [ ] **Tiva GPIO**：**RCGCGPIO** 开时钟 · **PF1/2/3 RGB** · **地址掩码写 DATA**
- [ ] 对照 [Ch5 位带](../chapter-05-loads-stores-addressing/notes/section-5-6-bit-banded.md) 理解 **单 pin 安全写**
- [ ] 联想 [21 驱动](../../../21-Linux-Device-Driver/) **`readl/writel`** 与 [20 U-Boot](../../../20-UBoot-Kernel-Build/) early UART

---

← [Ch 15](../chapter-15-exception-handling-v7m/) · 下一章 [Ch 17](../chapter-17-arm-thumb-thumb2-instructions/) · [OUTLINE](../OUTLINE.md) · [19 README](../../README.md)
