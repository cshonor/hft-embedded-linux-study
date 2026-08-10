# 3.1 Load-Store 核心规则

> 来源：§3.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

AArch64 是 Load-Store 架构——只有 LDR/STR 能访存，ALU 只能操作寄存器。

## 核心要点

1. 算术/移位/位运算 → **只能操作寄存器**，不能直接读写内存
2. `LDR`：内存→寄存器；`STR`：寄存器→内存
3. A64 指令 **固定 32-bit 定长**

对比 x86：x86 可 `add eax, [mem]`（内存直接参与运算）；ARM64 必须先 LDR → 运算 → STR。

## HFT 关联

Load-Store 架构对 HFT 性能的影响：
- 更多指令但更简单的解码 → 适合流水线，IPC 可预测
- 显式的内存加载让编译器/程序员更好控制 cache 行为
- 无法内存直接运算意味着中间值在寄存器中 → 减少不必要的内存访问
- 但如果寄存器压力大，可能产生 spill（寄存器溢出到栈），增加内存延迟

## 自测题

1. 为什么 ARM64 叫 Load-Store 架构？
<details><summary>答案</summary>
因为只有 LDR/STR 指令能访问内存，所有算术/逻辑运算只能在寄存器之间进行。内存数据必须先加载到寄存器才能参与运算。
</details>

2. 以下 x86 代码在 ARM64 上需要几条指令？
```x86
add eax, [rbx]
```
<details><summary>答案</summary>
3 条：`ldr w1, [x2]` → `add w0, w0, w1` → `str w0, [x2]`（如果需要写回）。ARM64 不能内存直接参与运算。
</details>

3. A64 指令为什么是 32 位定长？
<details><summary>答案</summary>
简化指令解码和流水线设计。定长指令不需要对齐检测和变长解码逻辑，减少解码延迟，适合高频流水线。
</details>

## 参考与延伸

- 原书 §3.1
- [3.2 寄存器宽度](02-register-width.md)
- ARM ARM §C1.2
