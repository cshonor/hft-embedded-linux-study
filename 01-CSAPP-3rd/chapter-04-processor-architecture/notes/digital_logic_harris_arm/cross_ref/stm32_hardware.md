# Cross-ref · Digital logic ↔ STM32 / embedded Linux

> **Core Concept:** Same digital + ARM story explains why kernel/drivers look the way they do
> **Link Target:** STM32 GPIO / timer / interrupt · Linux MMIO / IRQ · Harris Ch6/Ch7/Ch9

| Digital / ARM block | Embedded taste |
|---------------------|----------------|
| GPIO pin high/low | Combinational drive + register sample |
| Timer / counter | Sequential + clock enable |
| EXTI / NVIC / GIC | Async event → sync FF → ISR（控制流上像「打断再回来」） |
| Flash / SRAM / cache | Memory arrays + Ch8 hierarchy |
| MMIO + RPi labs | 外设 = 地址空间里的寄存器；驱动在写「硬件契约」 |

## Why this book helps embedded Linux

不只背手册位域：先懂 **组合/时序/总线/中断/内存映射**，再看内核 driver 框架，才能理解 **为什么** 要 ioremap、为什么要屏障、为什么中断下半部这么切。

## Notes

（读 Ch3/Ch6/Ch9 时往这里填；不写完整驱动教程。）
