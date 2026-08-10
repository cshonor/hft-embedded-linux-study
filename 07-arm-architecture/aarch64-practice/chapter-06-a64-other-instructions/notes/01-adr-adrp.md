# 6.1 ADR / ADRP 内核重定位关键

> 来源：§6.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

ADR 和 ADRP 指令——PC 相对地址加载，内核重定位和位置无关代码的关键。

## 核心要点

| 指令 | 作用 | 范围 |
|------|------|------|
| ADR | PC + 立即数偏移 → 寄存器 | ±1MB |
| ADRP | PC（页对齐）+ 立即数偏移 << 12 → 寄存器 | ±4GB |

```asm
adr x0, label       ; x0 = PC + offset（精确地址）
adrp x0, label      ; x0 = (PC & ~0xFFF) + (offset << 12)（页地址）
add x0, x0, :lo12:label  ; 补全低 12 位
```

- ADRP 计算 **4KB 页对齐**的基地址（低 12 位为 0）
- 需要加 `:lo12:label` 才能得到精确地址
- 内核重定位（KASLR）依赖 ADRP+ADD 的 PC 相对寻址

## HFT 关联

ADRP 在共享库和位置无关代码中至关重要：
- 交易引擎作为共享库加载时，全局变量访问用 ADRP+ADD → 支持 ASLR
- 内核模块（KO）加载用 ADRP 获取模块内数据 → 支持动态加载
- ADRP+ADD 比 LDR =伪指令更高效（不依赖文字池，无额外内存访问）
- 但 ADRP 跨页时需注意地址范围限制（±4GB）

## 自测题

1. ADR 和 ADRP 的区别？
<details><summary>答案</summary>
ADR 计算精确地址（PC + 偏移），范围 ±1MB。ADRP 计算页对齐地址（PC 页基址 + 偏移<<12），范围 ±4GB。ADRP 得到的地址低 12 位为 0，需要配合 ADD :lo12: 补全。
</details>

2. 为什么内核 KASLR 需要 ADRP 而不能用绝对地址？
<details><summary>答案</summary>
KASLR（内核地址空间随机化）在每次启动时改变内核加载地址。绝对地址在编译时固定，无法适应运行时重定位。ADRP 是 PC 相对寻址，不依赖绝对地址，内核代码无论加载到哪里都能正确找到自己的数据。
</details>

3. 以下代码的完整作用是什么？
```asm
adrp x0, global_var
add x0, x0, :lo12:global_var
```
<detail><summary>答案</summary>
获取 global_var 的运行时地址到 x0。ADRP 获取 global_var 所在的 4KB 页基地址，ADD 补全页内偏移（低 12 位）。这是 AArch64 获取全局变量地址的标准模式。
</details>

## 参考与延伸

- 原书 §6.1
- [Ch9 链接脚本](../../chapter-09-linker-scripts/notes/section-0-本章完整概述.md)
- [Ch14 MMU 页表](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
