# 6.7 易错点清单

> 来源：§6.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 4 大易错点

1. **ADRP 后忘记 ADD :lo12:** → 地址低 12 位为 0
2. **EL0 执行 MRS/MSR** → 触发非法异常
3. **LDXR/STXR 不循环重试** → 原子性不保证
4. **屏障指令放错位置** → 内存序错误

## 自测题

1. 只用 ADRP 不加 ADD，访问全局变量会发生什么？
<detail><summary>答案</summary>
ADRP 返回的是页对齐地址（低 12 位为 0）。如果全局变量不在页起始位置，直接用这个地址 LDR 会读到错误的数据——读到的是该页起始处的内容而非变量值。必须加 `ADD x0, x0, :lo12:label` 补全页内偏移。
</details>

2. 以下代码有什么问题？
```asm
ldxr w0, [x1]
add  w0, w0, #1
stxr w2, w0, [x1]
; 不检查 w2，继续执行
```
<detail><summary>答案</summary>
没有检查 STXR 的返回值 w2。如果 w2 != 0（独占写入失败），自增并没有成功。代码继续执行会导致计数器丢失更新。必须 `cbnz w2, retry` 重试。
</details>

3. 以下屏障使用有什么问题？
```asm
str x0, [data]       ; 写数据
str x1, [flag]       ; 写标志
dmb ish              ; 屏障放最后
```
<detail><summary>答案</summary>
屏障位置错误。CPU 可能把 flag 的写在 data 的写之前（弱序模型）。正确做法是在两次写之间放屏障：
```asm
str x0, [data]
dmb ish
str x1, [flag]
```
或者用 STLR 替代 str [flag] 自动带 release 语义。
</details>

## 参考与延伸

- 原书 §6.7
- [6.1 ADR/ADRP](01-adr-adrp.md)
- [Ch18 内存屏障](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md)
