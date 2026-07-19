# Cross-ref · Digital logic ↔ STM32 hardware

> **Core Concept:** Same digital blocks inside MCU peripherals
> **Link Target:** STM32 GPIO / timer / interrupt · Harris Ch3 / Ch7 skim

| Digital block | STM32 taste |
|---------------|-------------|
| GPIO pin high/low | Combinational drive + register sample |
| Timer / counter | Sequential + clock enable |
| EXTI / NVIC | Async event → sync FF → ISR (control-flow cousin of “flush”) |
| Flash / SRAM | Memory arrays (Harris §5.5) |

## Notes

（读 Ch3/Ch7 浅读时往这里填；不写驱动教程，只绑硬件直觉。）
