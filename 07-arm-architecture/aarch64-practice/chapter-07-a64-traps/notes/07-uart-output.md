# 7.7 串口输出实验

> 来源：§7.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

通过汇编实现串口（UART）输出，是最经典的裸机实验。

## 核心要点

PL011 UART（QEMU/Pi 使用）：
```asm
; UART 数据寄存器地址（QEMU virt: 0x09000000）
ldr x1, =0x09000000

; 输出一个字符 'H' (0x48)
mov w0, #'H'
str w0, [x1]
```

完整字符串输出：
```asm
print_string:
    ldr x1, =0x09000000   ; UART base
loop:
    ldrb w0, [x0], #1     ; 读字符，指针+1
    cbz w0, done           ; NULL 结尾
    str w0, [x1]           ; 写 UART
    b loop
done:
    ret
```

- UART 寄存器用 Device 内存属性 → 不可缓存
- str 直接写寄存器触发串口输出
- 实际应用中需检查 UART 状态寄存器（忙等待）

## HFT 关联

串口是裸机调试的"printf"：
- 无 OS 环境下唯一的调试输出手段
- 内核早期启动（MMU 未开）用串口调试
- HFT 系统的 bootloader/固件阶段用串口输出
- 但串口速度慢（115200 bps），不适合运行时高频日志

## 自测题

1. 为什么 UART 寄存器必须用 Device 内存属性？
<detail><summary>答案</summary>
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
<detail><summary>答案</summary>
1. QEMU 启动加 `-serial stdio` 或 `-nographic` 把串口重定向到终端
2. 运行裸机程序，终端应直接显示串口输出
3. 也可用 `-serial file:output.txt` 重定向到文件
4. GDB 断点在 STR 处，检查 w0 值确认字符正确
</details>

## 参考与延伸

- 原书 §7.7
- [3.1 Load-Store 规则](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- [Ch14 Device 内存属性](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
