# 6.7 易错点清单

> 来源：§6.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

杂项指令在实际编码中最容易犯的错误，包括 ADRP、系统寄存器、原子操作、内存屏障相关的陷阱。

## 七大易错点

### 1. ADRP 后忘记 ADD :lo12:

**错误**：ADRP 返回的是页对齐地址（低12位=0），直接用来访存会访问到错误的地址。

```asm
; BUG：ADRP 结果直接用
ADRP x0, global_var     ; x0 = 页基地址（低12位=0）
LDR  x1, [x0]           ; 错！读到的是页起始处的数据，不是 global_var

; 正确：补全页内偏移
ADRP x0, global_var
ADD  x0, x0, :lo12:global_var  ; x0 = 精确地址
LDR  x1, [x0]           ; 对！读到 global_var 的值

; 更简洁的写法
ADRP x0, global_var
LDR  x1, [x0, :lo12:global_var]  ; 直接在 LDR 中加偏移
```

### 2. EL0 执行 MRS/MSR

**错误**：用户态（EL0）没有权限访问系统寄存器。

```asm
; BUG：用户态代码中执行系统寄存器操作
; 在 EL0 执行：
MRS x0, SCTLR_EL1       ; → 触发同步异常 → SIGILL

; 正确：系统寄存器操作必须在 EL1+ 执行
; 用户态需要通过 SVC 系统调用让内核代为操作
```

### 3. LDXR/STXR 不循环重试

**错误**：STXR 可能失败，不检查返回值直接继续。

```asm
; BUG：不检查 STXR 返回值
LDXR W0, [X1]
ADD  W0, W0, #1
STXR W2, W0, [X1]
; 不检查 W2，继续执行 → 如果失败，自增未完成！

; 正确：必须循环重试
retry:
    LDXR W0, [X1]
    ADD  W0, W0, #1
    STXR W2, W0, [X1]
    CBNZ W2, retry      ; 失败则重试
```

### 4. 屏障指令放错位置

**错误**：屏障放在所有写之后，没有起到排序效果。

```asm
; BUG：屏障放在最后
STR x0, [data]
STR x1, [flag]
DMB ISH                 ; 太晚了！data 和 flag 之间没有排序

; 正确：屏障放在两次写之间
STR x0, [data]
DMB ISH
STR x1, [flag]          ; flag 的写一定在 data 之后

; 或用 STLR 自动排序
STR x0, [data]
STLR x1, [flag]         ; Store-Release 保证 data 先于 flag 可见
```

### 5. SVC 系统调用号放错寄存器

**错误**：把系统调用号放在 x0 而不是 x8。

```asm
; BUG：调用号在 x0
MOV x0, #64             ; write 的调用号
SVC #0
; 内核读 x8 查系统调用表，x8 是随机值 → 越界或调用错误函数

; 正确：调用号在 x8
MOV x8, #64             ; write 的调用号
MOV x0, #1              ; fd（第一个参数）
LDR x1, =msg            ; buffer（第二个参数）
MOV x2, #12             ; length（第三个参数）
SVC #0
```

### 6. 写系统寄存器后忘记 ISB

**错误**：修改 SCTLR/VBAR 等系统寄存器后不加 ISB。

```asm
; BUG：开 MMU 后不加 ISB
MRS x0, SCTLR_EL1
ORR x0, x0, #1          ; set MMU enable
MSR SCTLR_EL1, x0       ; 开启 MMU
; 下一条指令可能用旧的（MMU 关闭的）地址翻译 → 崩溃

; 正确：加 ISB
MRS x0, SCTLR_EL1
ORR x0, x0, #1
MSR SCTLR_EL1, x0
ISB                      ; 冲刷流水线，后续指令用新配置
```

### 7. 混淆 DMB 和 DSB 的使用场景

**错误**：DMA 场景用 DMB（不等待完成）而非 DSB。

```asm
; BUG：DMA 前用 DMB（不等待写入完成）
STR x0, [dma_buffer]    ; 写 DMA 数据
DMB ISH                  ; 排序但不等待 → 数据可能还在 write buffer 中
; 启动 DMA → DMA 可能读到旧数据

; 正确：DMA 前用 DSB（等待写入完成）
STR x0, [dma_buffer]
DSB ISH                  ; 等待数据真正写入内存
; 启动 DMA → 安全
```

## 易错点速查表

| 编号 | 易错点 | 正确做法 | 关键词 |
|------|--------|----------|--------|
| 1 | ADRP 不加 ADD :lo12: | 补全页内偏移 | 页对齐 |
| 2 | EL0 执行 MRS/MSR | 在 EL1+ 执行 | 权限 |
| 3 | STXR 不检查返回值 | CBNZ 循环重试 | 原子性 |
| 4 | DMB 放错位置 | 放在需要排序的两次访存之间 | 内存序 |
| 5 | SVC 调用号放错寄存器 | 调用号在 x8 | 约定 |
| 6 | 写系统寄存器后不加 ISB | 加 ISB 冲刷流水线 | 序列化 |
| 7 | DMA 场景用 DMB | 用 DSB 等待完成 | 同步 |

## 自测题

1. 只用 ADRP 不加 ADD，访问全局变量会发生什么？
<details><summary>答案</summary>
ADRP 返回的是页对齐地址（低 12 位为 0）。如果全局变量不在页起始位置，直接用这个地址 LDR 会读到错误的数据——读到的是该页起始处的内容而非变量值。必须加 `ADD x0, x0, :lo12:label` 补全页内偏移。
</details>

2. 以下代码有什么问题？
```asm
ldxr w0, [x1]
add  w0, w0, #1
stxr w2, w0, [x1]
; 不检查 w2，继续执行
```
<details><summary>答案</summary>
没有检查 STXR 的返回值 w2。如果 w2 != 0（独占写入失败），自增并没有成功。代码继续执行会导致计数器丢失更新。必须 `cbnz w2, retry` 重试。
</details>

3. 以下屏障使用有什么问题？
```asm
str x0, [data]       ; 写数据
str x1, [flag]       ; 写标志
dmb ish              ; 屏障放最后
```
<details><summary>答案</summary>
屏障位置错误。CPU 可能把 flag 的写在 data 的写之前（弱序模型）。正确做法是在两次写之间放屏障：
```asm
str x0, [data]
dmb ish
str x1, [flag]
```
或者用 STLR 替代 str [flag] 自动带 release 语义。
</details>

4. 以下代码会怎样？如何修复？
```asm
mrs x0, SCTLR_EL1
orr x0, x0, #1
msr SCTLR_EL1, x0
; 立即执行后续指令
ldr x1, [x2]
```
<details><summary>答案</summary>
写 SCTLR_EL1（开启 MMU）后不加 ISB，流水线中可能有旧指令使用 MMU 关闭时的地址翻译。执行 LDR 时如果 MMU 刚开启但流水线未刷新，地址翻译不确定 → 可能崩溃。修复：在 MSR 后加 ISB：
```asm
msr SCTLR_EL1, x0
isb                  ; 冲刷流水线，确保后续指令用新配置
ldr x1, [x2]         ; 此时 MMU 已生效
```
</details>

5. 为什么 DMA 前应该用 DSB 而不是 DMB？
<details><summary>答案</summary>
DMB 只排序不等待——数据可能还在 CPU 的 write buffer 中，尚未到达内存。DMA 直接从内存读取，可能读到旧数据。DSB 等待所有之前的访存操作真正完成（数据写入内存、对设备可见），然后才继续执行。所以 DMA 前必须用 DSB 确保数据已到达内存。
</details>

## 参考与延伸

- 原书 §6.7
- [6.1 ADR/ADRP](01-adr-adrp.md)
- [6.2 SVC](02-svc.md)
- [6.3 MRS/MSR](03-mrs-msr.md)
- [6.4 LDXR/STXR](04-ldxr-stxr-preview.md)
- [6.5 屏障](05-barrier-preview.md)
- [Ch18 内存屏障](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md)
