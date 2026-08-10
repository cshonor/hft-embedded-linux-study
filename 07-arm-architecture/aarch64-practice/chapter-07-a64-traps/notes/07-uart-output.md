# 7.7 串口输出实验

> 来源：§7.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

通过汇编实现串口（UART）输出——最经典的裸机实验，验证前几章学到的 Load-Store、循环、条件跳转等知识。

## 核心要点

### PL011 UART 简介

```
PL011 是 ARM PrimeCell UART，广泛用于：
  - QEMU virt 机器（地址 0x09000000）
  - Raspberry Pi（地址 0xFE201000）
  - 各类 ARM 开发板

关键寄存器（偏移量）：
  0x00  UART_DR    数据寄存器（写 → 发送，读 → 接收）
  0x18  UART_FR    标志寄存器
        bit5 TXFF  发送 FIFO 满（1=满，等待）
        bit4 RXFE  接收 FIFO 空（1=空）
  0x24  UART_LCRH  线控制寄存器（数据位/停止位/校验）
  0x30  UART_CR    控制寄存器（UARTEN 发送/接收使能）
  0x38  UART_IMSC  中断屏蔽寄存器
```

### 最简串口输出

```asm
; QEMU virt 机器，UART base = 0x09000000
UART_BASE = 0x09000000

; 输出单个字符 'H'
LDR X1, =UART_BASE
MOV W0, #'H'              ; 字符字面量
STR W0, [X1]              ; 写数据寄存器 → 字符输出
```

### 完整字符串输出函数

```asm
; print_string: 输出 X0 指向的 NULL 结尾字符串
; 输入: X0 = 字符串地址
; 输出: 无
; 破坏: X0, X1, X2
print_string:
    LDR X1, =0x09000000   ; UART base address
.loop:
    LDRB W2, [X0], #1     ; 读 1 字节字符，指针后变基 +1
    CBZ W2, .done          ; NULL 结尾 → 结束
.wait:
    LDR W3, [X1, #0x18]   ; 读 UART_FR（标志寄存器）
    TBNZ W3, #5, .wait    ; bit5=TXFF → 发送 FIFO 满，等待
    STR W2, [X1]           ; 写数据寄存器 → 发送字符
    B .loop
.done:
    RET
```

### 带忙等待的串口输出

```asm
; print_char: 输出单个字符（带忙等待）
; 输入: W0 = 字符
print_char:
    LDR X1, =0x09000000   ; UART base
.wait:
    LDR W2, [X1, #0x18]   ; 读 UART_FR
    TBNZ W2, #5, .wait    ; TXFF=1 → 等待发送 FIFO 有空间
    STR W0, [X1]           ; 写 UART_DR → 发送字符
    RET
```

### 数字输出（十进制）

```asm
; print_decimal: 输出 X0 的十进制表示
; 输入: X0 = 数字
print_decimal:
    MOV SP, SP             ; 保存 SP（简化版，实际需保存）
    MOV X1, #10
    MOV X2, #0             ; 数字位数计数

    ; 特殊情况：0
    CBZ X0, .print_zero

    ; 逐位除以 10，余数入栈
.div_loop:
    CBZ X0, .print_digits
    UDIV X3, X0, X1        ; X3 = X0 / 10
    MSUB X4, X3, X1, X0    ; X4 = X0 - X3*10 = X0 % 10
    MOV X0, X3              ; X0 = X0 / 10
    ADD W4, W4, #'0'       ; 转 ASCII
    STRB W4, [SP, #-1]!    ; 字符入栈
    ADD X2, X2, #1          ; 位数++
    B .div_loop

.print_digits:
    CBZ X2, .done
    LDRB W0, [SP], #1      ; 出栈一个字符
    BL print_char
    SUB X2, X2, #1
    B .print_digits

.print_zero:
    MOV W0, #'0'
    BL print_char
.done:
    RET
```

### 十六进制输出

```asm
; print_hex: 输出 X0 的 16 位十六进制表示
; 输入: X0 = 数字
print_hex:
    LDR X1, =0x09000000
    MOV X2, #60            ; 移位量（从最高 4 位开始）
    MOV X3, #16            ; 循环 16 次（64位/4位）
.hex_loop:
    LSR X4, X0, X2         ; 右移取 4 位
    AND X4, X4, #0xF       ; 取低 4 位
    CMP X4, #10
    B.LO .digit            ; 0-9 → '0'+n
    ADD W4, W4, #('A' - 10); A-F
    B .out
.digit:
    ADD W4, W4, #'0'       ; 0-9
.out:
    STR W4, [X1]           ; 输出字符
    SUB X2, X2, #4          ; 移位量 -= 4
    SUBS X3, X3, #1
    B.NE .hex_loop
    RET
```

### 完整裸机程序

```asm
; hello_world.s — QEMU virt 机器裸机 Hello World
.section .text
.global _start

_start:
    ADR X0, msg            ; X0 = 字符串地址
    BL print_string        ; 输出

hang:
    WFE                    ; 等待事件（低功耗循环）
    B hang

msg:
    .asciz "Hello, AArch64!\n"

print_string:
    LDR X1, =0x09000000
.loop:
    LDRB W2, [X0], #1
    CBZ W2, .done
.wait:
    LDR W3, [X1, #0x18]
    TBNZ W3, #5, .wait
    STR W2, [X1]
    B .loop
.done:
    RET
```

**编译和运行**：
```bash
# 编译
aarch64-linux-gnu-as hello_world.s -o hello.o
aarch64-linux-gnu-ld -Ttext 0x40000000 hello.o -o hello.elf

# 运行
qemu-system-aarch64 -M virt -cpu cortex-a72 -kernel hello.elf -nographic
# 终端输出: Hello, AArch64!
```

## 与 C 的对照

```c
// 等价的 C 代码（裸机，无 OS）
#define UART0_BASE 0x09000000
#define UART_DR    (*(volatile unsigned int *)(UART0_BASE + 0x00))
#define UART_FR    (*(volatile unsigned int *)(UART0_BASE + 0x18))
#define UART_FR_TXFF (1 << 5)

void uart_putc(char c) {
    while (UART_FR & UART_FR_TXFF) ;  // 忙等待
    UART_DR = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}
```

## 常见错误

1. **不检查 UART_FR 的 TXFF 位**：直接写 UART_DR，发送 FIFO 满时丢数据。
2. **用 LDR W 而非 LDRB 读字符串**：LDR W 一次读 4 字节，可能越界。
3. **UART 地址错误**：QEMU virt（0x09000000）和 Raspberry Pi（0xFE201000）地址不同。

## HFT 关联

串口是裸机调试的"printf"：
- 无 OS 环境下唯一的调试输出手段
- 内核早期启动（MMU 未开）用串口调试
- HFT 系统的 bootloader/固件阶段用串口输出
- 但串口速度慢（115200 bps ≈ 14 KB/s），不适合运行时高频日志
- HFT 运行时调试推荐用共享内存 + 日志 ring buffer

## 自测题

1. 为什么 UART 寄存器必须用 Device 内存属性？
<details><summary>答案</summary>
UART 寄存器是 MMIO 外设。如果用 Normal 可缓存属性，CPU 可能缓存写入而不真正发送到外设，或者合并/重排写入导致数据错误。Device 属性保证每次 STR 真正到达外设，且严格保序、不合并。
</details>

2. 以下串口输出代码有什么问题？
```asm
loop:
    ldr w0, [x1]       ; 读字符
    str w0, [x2]       ; 写 UART
    b loop
```
<details><summary>答案</summary>
1. 没有检查字符串结尾（NULL）→ 无限循环
2. 没有检查 UART 发送缓冲区是否就绪 → 可能丢数据
3. ldr w0 应该是 ldrb w0（字符是 1 字节，读 4 字节会越界）
</details>

3. 如何在 QEMU 上验证串口输出？
<details><summary>答案</summary>
1. QEMU 启动加 `-serial stdio` 或 `-nographic` 把串口重定向到终端
2. 运行裸机程序，终端应直接显示串口输出
3. 也可用 `-serial file:output.txt` 重定向到文件
4. GDB 断点在 STR 处，检查 w0 值确认字符正确
</details>

4. 为什么串口不适合 HFT 运行时高频日志？
<details><summary>答案</summary>
串口波特率 115200 bps ≈ 14 KB/s，输出一个 100 字节的日志需要 ~7ms。HFT 热路径的延迟预算通常在微秒级，串口日志的延迟会严重拖慢系统。替代方案：
1. 共享内存 + ring buffer（纳秒级写入）
2. ftrace/perf（内核级追踪）
3. 串口只用于启动阶段和严重错误报告
</details>

5. print_string 函数中 `LDRB W2, [X0], #1` 的后变基有什么作用？
<details><summary>答案</summary>
`LDRB W2, [X0], #1` 一步完成：读取 X0 指向的 1 字节到 W2，然后 X0 += 1（指针后移到下一个字符）。后变基省去了一条单独的 ADD 指令更新指针。这在字符串遍历、内存拷贝等循环中是标准优化模式，每轮循环少 1 条指令。
</details>

## 参考与延伸

- 原书 §7.7
- [3.1 Load-Store 规则](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- [3.7 典型模式（后变基）](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- [Ch14 Device 内存属性](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
