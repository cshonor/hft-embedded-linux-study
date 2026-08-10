# 3.5 LDR 伪指令 ldr =label

> 来源：§3.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

真 LDR 机器指令和 `ldr x0, =label` 伪指令的区别，以及 MOV 的立即数限制。

## 核心要点

| 写法 | 是什么 |
|------|--------|
| `ldr x0, [x1]` | **真机器指令**，访存 |
| `ldr x0, =val` | **GNU 伪指令**，汇编器生成文字池(litpool)+真 LDR |

MOV 限制：大体只能 16 位立即数，或移位 16/32/48 位。多数 64 位大常量 → `ldr x0, =xxx`。

底层原理：
1. 汇编器把常量 `val` 放进 `.litpool`（文字池，通常在函数末尾）
2. 生成 `ldr x0, [pc, #offset]` 从文字池读取
3. 链接器解析 PC 相对偏移

## HFT 关联

伪指令的性能影响：
- `ldr =` 会产生额外的内存访问（从文字池加载）→ 比立即数 MOV 多一个 cache miss 风险
- 如果文字池不在 L1 cache → 额外 ~12 cycles (L2) 的延迟
- 对于热路径中的常量，优先用 `mov`/`movz`/`movk` 组合（如果常量可以用 16 位分段表示）
- 编译器通常自动选择最优方式，但内联汇编中需注意

## 自测题

1. `ldr x0, =0x12345678deadbeef` 是一条机器指令吗？
<details><summary>答案</summary>
不是。它是 GNU 伪指令。汇编器会把这个 64 位常量放进文字池，然后生成一条真 LDR 指令从文字池（PC 相对地址）加载。
</details>

2. 如何用 MOV 系列指令加载 0x12345678？
<details><summary>答案</summary>
```asm
movz x0, #0x5678        ; 低 16 位
movk x0, #0x1234, lsl #16  ; 高 16 位
```
两条 MOV 指令即可，不需要文字池。
</details>

3. 为什么 A64 不支持任意 64 位立即数的单条 MOV？
<details><summary>答案</summary>
A64 指令固定 32 位，其中操作码占用若干位，剩余位不足以编码 64 位立即数。只能编码 16 位立即数+移位选项，所以大常量需要 MOVK 组合或文字池。
</details>

## 参考与延伸

- 原书 §3.5
- [3.6 特殊访存](06-special-load-store.md)
- [Ch7 MOV 陷阱](../../chapter-07-a64-traps/notes/section-0-本章完整概述.md)
