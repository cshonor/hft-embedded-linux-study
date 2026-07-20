# Ch6 · 体系结构（ARM ISA + x86 对照）

> **Core Concept:** ARM assembly & machine language; Thumb/evolution; **x86 perspective**
> **Link Target:** Embedded Linux on ARM · CSAPP Ch3 x86 · feeds Ch7 microarch

## 目录锚点

- 6.2 汇编：指令与操作数
- 6.3 编程：数据处理、标志、分支、循环、访存、函数调用
- 6.4 机器语言与寻址
- `*6.5` 编译汇编链接加载 · `*6.6` 其他 — 可选
- 6.7 ARM 演变：Thumb、DSP、FP、SIMD、64 位…
- **6.8 另一个视角：x86** — 对接 CSAPP / HFT 真机
- 6.9 总结

## Status

- [ ] 6.2–6.4 够读 Ch7 数据通路
- [ ] 6.8 x86 对照笔记
- [ ] `../cross_ref/stm32_hardware.md`

## Notes

**本章 = ISA，不是流水线。** 单周期/流水线/冲突 → [ch7_microarchitecture.md](./ch7_microarchitecture.md)。
