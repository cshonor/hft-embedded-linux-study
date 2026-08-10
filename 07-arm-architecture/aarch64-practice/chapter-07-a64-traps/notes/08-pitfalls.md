# 7.8 易错点清单

> 来源：§7.8 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

本章涉及的所有工程陷阱的汇总清单，作为快速参考和自测。

## 七大易错点

### 1. MOV 大立即数超出 16 位

```asm
; BUG：超出 16 位立即数
MOV X0, #0x12345678      ; ❌ 汇编报错

; 正确：MOVZ + MOVK
MOVZ X0, #0x5678
MOVK X0, #0x1234, LSL #16

; 或用 LDR =伪指令
LDR X0, =0x12345678
```

### 2. 字符串加载小端序混淆

```asm
; BUG：以为 LDR ='ABCD' 得到 0x41424344
LDR W0, ='ABCD'         ; 实际 W0 = 0x44434241（小端反转！）

; 注意：小端模式下 'A' 在最低位，'D' 在最高位
; 如果需要特定字节序，手动构造
```

### 3. LDXR 在 Device 内存上执行

```asm
; BUG：在 MMIO 地址上用 LDXR
LDR X0, =0x09000000     ; UART 寄存器（Device 内存）
LDXR W1, [X0]           ; ❌ 死机

; 正确：MMIO 用普通 LDR/STR
LDR W1, [X0]            ; ✓ 安全
; 原子操作只在 Normal Cacheable 内存上使用
```

### 4. SP 不 16 字节对齐

```asm
; BUG：分配非 16 倍数栈空间
SUB SP, SP, #8          ; ❌ 破坏 16 字节对齐
STR X0, [SP]            ; ❌ SP 对齐异常

; 正确：始终分配 16 的倍数
SUB SP, SP, #16         ; ✓
STP X0, X1, [SP]        ; ✓
```

### 5. AArch32 条件执行搬到 AArch64

```asm
; BUG：AArch32 风格条件指令
MOVEQ X0, #1            ; ❌ AArch64 不支持
ADDGT X0, X0, #1        ; ❌ AArch64 不支持

; 正确：用 CSEL/CSET/CINC
CSET X0, EQ             ; ✓ 条件置 1
CINC X0, X0, GT         ; ✓ 条件自增
```

### 6. 开 MMU 后忘记 ISB

```asm
; BUG：开 MMU 后直接执行后续指令
MSR SCTLR_EL1, X0       ; 开启 MMU
LDR X1, [X2]            ; ❌ 可能用旧地址翻译 → 崩溃

; 正确：加 ISB
MSR SCTLR_EL1, X0
ISB                      ; ✓ 冲刷流水线
LDR X1, [X2]            ; ✓ 用新 MMU 配置
```

### 7. 串口输出不检查 TXFF

```asm
; BUG：直接写 UART 不检查 FIFO 状态
STR W0, [X1]            ; ❌ FIFO 满时丢数据

; 正确：检查 TXFF 位
.wait:
    LDR W2, [X1, #0x18] ; 读 UART_FR
    TBNZ W2, #5, .wait  ; TXFF=1 → 等待
    STR W0, [X1]         ; ✓ FIFO 有空间
```

## 易错点速查表

| 编号 | 易错点 | 正确做法 | 影响 |
|------|--------|----------|------|
| 1 | MOV 大立即数 | MOVZ+MOVK 或 LDR = | 编译报错 |
| 2 | 字符串端序 | 注意小端反转 | 数据错误 |
| 3 | LDXR 在 Device | 只在 Normal 内存用 | 死机 |
| 4 | SP 不对齐 | 始终 16 的倍数 | SP 异常 |
| 5 | AArch32 条件指令 | 用 CSEL/CSET 替代 | 编译报错 |
| 6 | 开 MMU 不加 ISB | MSR 后跟 ISB | 崩溃 |
| 7 | 串口不检查 TXFF | 轮询 UART_FR | 数据丢失 |

## 自测题

1. 以下代码哪里有错？
```asm
mov x0, #0x12345678
str x0, [sp]
```
<details><summary>答案</summary>
`mov x0, #0x12345678` 不合法——0x12345678 超出 16 位立即数范围，不能单条 MOV 加载。应改为 `movz x0, #0x5678; movk x0, #0x1234, lsl #16` 或 `ldr x0, =0x12345678`。
</details>

2. 在 QEMU 上 LDXR/STXR 测试通过，上真实 Pi 硬件却死机，可能原因？
<details><summary>答案</summary>
1. 在 Device/MMIO 内存上用了 LDXR（QEMU 不检查，真实硬件会异常）
2. 地址未对齐（QEMU 宽松，硬件严格）
3. 独占监视器超时（QEMU 不模拟超时，硬件有超时限制）
4. QEMU 没有完整实现独占监视器竞争 → 真实硬件 STXR 失败处理不当
</details>

3. 从 ARMv7 代码迁移 `itt eq; mov r0, #1; mov r1, #2` 到 AArch64 怎么写？
<details><summary>答案</summary>
AArch64 没有 IT 块，需要用 CSEL 或分支：
```asm
// 方法1：分支
b.ne skip
mov x0, #1
mov x1, #2
skip:

// 方法2：CSEL（如果只需要选值）
mov x2, #1
mov x3, #0
csel x0, x2, x3, eq   ; eq → x0=1, ne → x0=0
```
</details>

4. 以下代码在 QEMU 上运行正常，但在树莓派上崩溃。找出原因。
```asm
_start:
    ldr x0, =0x09000000   ; UART base
    ldxr x1, [x0]         ; 读取 UART 数据
    add x1, x1, #1
    stxr w2, x1, [x0]     ; 写回
```
<details><summary>答案</summary>
在 Device 内存（UART MMIO）上使用 LDXR/STXR。QEMU 不检查 Device 内存上的独占访问，但真实硬件会触发异常。修复：MMIO 寄存器用普通 LDR/STR：
```asm
ldr x1, [x0]     ; 普通读
add x1, x1, #1
str x1, [x0]     ; 普通写
```
</details>

5. 以下函数有什么问题？
```asm
func:
    sub sp, sp, #24       ; 分配 24 字节
    stp x29, x30, [sp]    ; 保存帧指针
    stp x0, x1, [sp, #16] ; 保存参数
    ...
    add sp, sp, #24
    ret
```
<details><summary>答案</summary>
`SUB SP, SP, #24` 破坏了 16 字节对齐——24 不是 16 的倍数。24 = 16 + 8，SP 变成 8 字节对齐。后续的 `STP X29, X30, [SP]` 是 16 字节操作，在不对齐的 SP 上会触发 SP 对齐异常（如果 SA/SA0 开启）。修复：分配 32 字节（16 的倍数）：
```asm
sub sp, sp, #32        ; 32 = 16×2 ✓
stp x29, x30, [sp]
stp x0, x1, [sp, #16]
...
add sp, sp, #32
ret
```
</details>

## 参考与延伸

- 原书 §7.8
- [7.1 MOV 陷阱](01-mov-trap.md)
- [7.2 字符串加载](02-string-load.md)
- [7.3 LDXR 死机](03-ldxr-crash.md)
- [7.4 栈对齐](04-stack-alignment.md)
- [7.5 条件执行](05-condition-trap.md)
