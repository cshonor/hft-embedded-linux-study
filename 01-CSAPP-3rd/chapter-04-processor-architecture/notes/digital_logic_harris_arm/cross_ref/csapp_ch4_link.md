# Cross-ref · Harris ARM ↔ CSAPP Chapter 4

> **Core Concept:** Map Harris blocks to CSAPP §4.x (correct ARM Edition chapter numbers)
> **Link Target:** [CSAPP Ch4 README](../../../README.md) · §4.2–§4.5 notes

| Harris (ARM Ed.) | CSAPP | One-line link |
|------------------|-------|---------------|
| Ch2 MUX / timing | [§4.2](../../section-4.2-HCL逻辑与组合电路.md) | `sel` + propagation delay |
| Ch3 D FF / setup-hold | §4.2 sequential · [§4.4](../../section-4.4-流水线原理与局限.md) pipe regs | Latch between stages |
| Ch5 ALU / SRAM | §4.2 ALU | Adder + logic + result MUX |
| Ch6 Architecture (ARM ISA) | Ch3 contrast (x86 vs ARM) | Programmer model only |
| **Ch7** single-cycle | [§4.3 SEQ](../../section-4.3-SEQ顺序处理器.md) | One instr completes F…W |
| **Ch7** pipeline + hazards | [§4.4](../../section-4.4-流水线原理与局限.md) · [§4.5](../../section-4.5-PIPE流水线与冒险.md) | Forwarding / stall / flush |
| Ch8 caches | CSAPP Ch6 (later) | Locality ↔ HFT |
| Ch9 I/O + RPi | STM32 I/O intuition | MMIO + interrupts |

## Fill as you read

（对照表扩写、易混点、图号。）
