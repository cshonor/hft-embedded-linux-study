# 3.8 易错点清单

> 来源：§3.8 · 精读 · [章总览](section-0-本章完整概述.md)

## 5 大易错点

1. **ALU 不能碰内存**（Load-Store 架构）
2. **真 LDR vs `ldr =` 伪指令**（前者访存，后者文字池）
3. **`!` 前变基 vs 后变基**（基址会不会改）
4. **无 PUSH/POP** → STP/LDP
5. **LDRSB 符号扩展 vs LDRB 零扩展**

## 自测题

1. 以下两条指令哪个是真机器指令？
```asm
ldr x0, [x1]
ldr x0, =0x80000000
```
<details><summary>答案</summary>
第一条 `ldr x0, [x1]` 是真机器指令（访存）。第二条是伪指令，汇编器会生成文字池 + 真 LDR。
</details>

2. `stp x0, x1, [sp, #-16]`（没有 `!`）执行后 sp 变了吗？
<details><summary>答案</summary>
没变。没有 `!` 表示基址偏移模式，只访问 `[sp-16]` 地址但不写回 sp。要修改 sp 需要加 `!`：`stp x0, x1, [sp, #-16]!`。
</details>

3. 读取有符号字节 -1，用 LDRB 会得到什么？
<details><summary>答案</summary>
LDRB 是零扩展，读 0xFF → 0x000000FF(255) 而非 -1。要正确读取有符号负数必须用 LDRSB（符号扩展）→ 0xFFFFFFFF(-1)。
</details>

## 参考与延伸

- 原书 §3.8
- [Ch7 工程陷阱](../../chapter-07-a64-traps/notes/section-0-本章完整概述.md)
