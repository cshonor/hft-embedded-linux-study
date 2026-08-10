# 8.6 实验要点

> 来源：§8.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

通过 QEMU 实验练习 GNU as 汇编器的使用：编译、链接、反汇编、C↔汇编混合编程。

## 实验列表

| 实验 | 内容 | 平台 | 重点 |
|------|------|------|------|
| 8-1 | GNU as 基本语法练习 | QEMU | 标号、注释、立即数 |
| 8-2 | 伪指令和段的使用 | QEMU | .data/.bss/.align |
| 8-3 | 宏定义和使用 | QEMU | .macro/.endm |
| 8-4 | C 调用汇编函数 | QEMU | AAPCS64 约定 |
| 8-5 | 汇编调用 C 函数 | QEMU | BL 调用 C |

## 实验 8-1：汇编求最大数

**目标**：编写汇编程序求三个数中的最大值。

```asm
; max3.s — 求三个数的最大值
.section .text
.global _start

_start:
    mov x0, #15          ; a = 15
    mov x1, #42          ; b = 42
    mov x2, #7           ; c = 7

    ; max(a, b) → x3
    cmp x0, x1
    csel x3, x0, x1, ge  ; x3 = max(a, b)

    ; max(x3, c) → x0
    cmp x3, x2
    csel x0, x3, x2, ge  ; x0 = max(x3, c) = max(a, b, c)

    ; x0 = 42（最大值）
    ; exit(42)
    mov x8, #93          ; syscall: exit
    svc #0
```

**编译运行**：
```bash
aarch64-linux-gnu-as max3.s -o max3.o
aarch64-linux-gnu-ld -o max3 max3.o
qemu-aarch64 ./max3; echo $?   # 输出 42
```

## 实验 8-2：伪指令和段

**目标**：使用 .data/.bss/.align/.word 等伪指令。

```asm
; data_demo.s
.section .data
    .align 3             ; 8字节对齐
values:
    .word 10, 20, 30, 40, 50    ; 5个4字节整数

message:
    .asciz "Sum = "

.section .bss
    .align 3
sum_buf:
    .space 8             ; 未初始化的8字节缓冲区

.section .text
.global _start
_start:
    ; 累加 values 数组
    adr x0, values
    mov x1, #0           ; sum = 0
    mov x2, #5           ; count = 5
    mov x3, #0           ; i = 0
loop:
    cmp x3, x2
    b.ge done
    ldr w4, [x0, x3, lsl #2]  ; values[i]（4字节）
    add x1, x1, x4
    add x3, x3, #1
    b loop
done:
    ; x1 = 150（总和）
    ; 存入 sum_buf
    adr x5, sum_buf
    str x1, [x5]

    ; exit(150)
    mov x0, x1
    mov x8, #93
    svc #0
```

**反汇编查看**：
```bash
aarch64-linux-gnu-objdump -d data_demo   # 查看代码段
aarch64-linux-gnu-objdump -s -j .data data_demo  # 查看数据段
```

## 实验 8-3：宏定义

**目标**：用宏简化寄存器保存/恢复。

```asm
; macro_demo.s
.macro SAVE_PAIR r1, r2
    stp \r1, \r2, [sp, #-16]!
.endm

.macro LOAD_PAIR r1, r2
    ldp \r1, \r2, [sp], #16
.endm

.section .text
.global _start
_start:
    SAVE_PAIR x19, x20       ; 展开为 stp x19, x20, [sp, #-16]!
    SAVE_PAIR x21, x22

    mov x19, #100
    mov x20, #200
    add x0, x19, x20         ; x0 = 300

    LOAD_PAIR x21, x22       ; 恢复
    LOAD_PAIR x19, x20

    mov x8, #93
    svc #0
```

**验证宏展开**：
```bash
aarch64-linux-gnu-as macro_demo.s -o macro_demo.o
aarch64-linux-gnu-objdump -d macro_demo.o
# 确认 STP/LDP 指令已展开
```

## 实验 8-4：C 调用汇编函数

**目标**：C 代码调用汇编实现的函数。

```c
// main_c.c
#include <stdio.h>
extern int asm_max(int a, int b);
extern int asm_abs(int a);

int main() {
    printf("max(3, 7) = %d\n", asm_max(3, 7));
    printf("abs(-5) = %d\n", asm_abs(-5));
    return 0;
}
```

```asm
// asm_funcs.s
.global asm_max
asm_max:
    cmp x0, x1
    csel x0, x0, x1, ge     ; max(a, b)
    ret

.global asm_abs
asm_abs:
    cmp x0, #0
    cneg x0, x0, mi          ; abs(a)
    ret
```

**编译运行**：
```bash
aarch64-linux-gnu-gcc -c main_c.c -o main_c.o
aarch64-linux-gnu-as asm_funcs.s -o asm_funcs.o
aarch64-linux-gnu-gcc main_c.o asm_funcs.o -o test_c
qemu-aarch64 ./test_c
# 输出: max(3, 7) = 7
#       abs(-5) = 5
```

## 实验 8-5：汇编调用 C 函数

**目标**：汇编代码调用 C 库函数。

```asm
// call_c.s
.section .text
.global _start
_start:
    ; 设置 printf 参数
    adr x0, fmt             ; format string
    mov x1, #42             ; value
    bl  printf              ; 调用 C 库函数

    ; exit(0)
    mov x0, #0
    mov x8, #93
    svc #0

.section .rodata
fmt:
    .asciz "Answer = %d\n"
```

**编译运行**：
```bash
aarch64-linux-gnu-gcc -static call_c.s -o call_c
qemu-aarch64 ./call_c
# 输出: Answer = 42
```

## 自测题

1. 如何用命令行把 .s 文件编译为可执行文件？
<details><summary>答案</summary>
```bash
aarch64-linux-gnu-gcc -c test.s -o test.o
aarch64-linux-gnu-gcc test.o -o test
```
或直接用 gcc 编译汇编+链接：
```bash
aarch64-linux-gnu-gcc test.s -o test
```
</details>

2. `objdump -d` 的输出中如何判断某行是代码还是数据？
<details><summary>答案</summary>
`objdump -d` 只反汇编代码段（.text）。数据段需要 `objdump -s -j .data` 查看。如果 .text 中混入了数据（如文字池），objdump 会尝试反汇编，产生看起来像指令但实际是数据的输出。
</details>

3. C 调用汇编函数时，链接器怎么知道函数地址？
<details><summary>答案</summary>
汇编函数用 `.global func_name` 声明为全局符号。链接器在符号表中查找，解析调用地址。没有 `.global` 声明的符号是局部的，其他文件无法引用。
</details>

4. 如何在 GDB 中调试汇编程序？
<details><summary>答案</summary>
```bash
aarch64-linux-gnu-gdb test
(gdb) b _start              ; 在入口设断点
(gdb) run
(gdb) si                     ; 单步执行一条指令
(gdb) info registers         ; 查看所有寄存器
(gdb) p/x $x0                ; 查看特定寄存器
(gdb) disas                   ; 反汇编当前函数
```
</details>

5. 以下命令各做什么？
```bash
aarch64-linux-gnu-as test.s -o test.o
aarch64-linux-gnu-ld test.o -o test
aarch64-linux-gnu-objdump -d test
```
<details><summary>答案</summary>
- `as test.s -o test.o`：汇编，将 .s 源文件编译为目标文件（.o），包含机器码但未链接。
- `ld test.o -o test`：链接，将目标文件链接为可执行文件。如果用了 C 库函数，需要用 `gcc` 而非 `ld` 来链接（自动链接 libc）。
- `objdump -d test`：反汇编，将可执行文件中的代码段反汇编为汇编指令，用于检查生成的机器码是否正确。
</details>

## 参考与延伸

- 原书 §8.6
- [8.5 C↔汇编互调](05-c-asm-interop.md)
- [8.1 基本语法](01-as-syntax.md)
