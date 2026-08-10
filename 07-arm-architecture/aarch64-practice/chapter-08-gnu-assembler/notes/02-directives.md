# 8.2 常用伪指令/伪操作

> 来源：§8.2 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

GNU as 常用伪操作（directives）：.global/.text/.data/.align/.word/.quad/.string 等。伪操作不是真正的机器指令，而是汇编器提供的便利写法。

## 核心要点

### 完整伪操作表

| 伪操作 | 作用 | 示例 |
|--------|------|------|
| `.global sym` | 声明全局符号 | `.global _start` |
| `.text` | 切换到代码段 | `.section .text` |
| `.data` | 切换到数据段 | `.section .data` |
| `.bss` | 切换到 BSS 段 | `.section .bss` |
| `.align n` | 对齐 2^n 字节 | `.align 4` → 16字节对齐 |
| `.balign n` | 对齐 n 字节（跨架构统一） | `.balign 16` → 16字节 |
| `.word val` | 写 4 字节 | `.word 0x12345678` |
| `.quad val` | 写 8 字节 | `.quad 0xDEADBEEFCAFEBABE` |
| `.hword val` | 写 2 字节 | `.hword 0x1234` |
| `.byte val` | 写 1 字节 | `.byte 0x41` |
| `.string "str"` | 写字符串（含 NULL） | `.string "Hello"` → 6字节 |
| `.ascii "str"` | 写字符串（不含 NULL） | `.ascii "Hello"` → 5字节 |
| `.asciz "str"` | 同 .string | `.asciz "Hello"` → 6字节 |
| `.space n` | 填充 n 字节零 | `.space 64` |
| `.zero n` | 填充 n 字节零 | `.zero 64` |
| `.equ name, val` | 定义常量 | `.equ BUF_SIZE, 1024` |
| `.include "file"` | 包含文件 | `.include "macro.s"` |
| `.macro/.endm` | 宏定义 | 见 §8.4 |
| `.if/.else/.endif` | 条件汇编 | `.if \count > 0` |

### .align 详解（重要差异）

```asm
; .align 在不同架构上含义不同！

; AArch64: .align n → 对齐到 2^n 字节
.align 4                ; 对齐到 2^4 = 16 字节
.align 6                ; 对齐到 2^6 = 64 字节（cache line）

; x86: .align n → 对齐到 n 字节
; .align 4 → 对齐到 4 字节（不是 16！）

; 跨架构安全做法：用 .balign
.balign 16              ; 所有架构都是 16 字节对齐
.balign 64              ; 所有架构都是 64 字节对齐
```

### 数据定义示例

```asm
.section .data

; 已初始化的全局变量
counter:
    .quad 0                    ; 8字节，初始值0

message:
    .asciz "Hello, AArch64!"   ; 带NULL的字符串

; 数组
lookup_table:
    .word 10, 20, 30, 40, 50   ; 5个4字节整数

; 字节缓冲区
buffer:
    .space 256                 ; 256字节，全零

; 对齐到 cache line（64字节）
.align 6
hot_data:
    .quad 0                    ; 64字节对齐的全局变量

.section .bss
; 未初始化的全局变量（不占二进制空间）
big_array:
    .space 4096                ; 4KB 数组，加载时零填充
```

### .equ 常量定义

```asm
.equ UART_BASE, 0x09000000     ; 定义常量
.equ BUF_SIZE, 256
.equ FLAG_READY, (1 << 0)

; 使用常量
    ldr x0, =UART_BASE         ; 加载地址
    mov x1, #BUF_SIZE          ; 加载大小
```

### LDR =伪指令

```asm
; LDR =是伪指令，不是真正的 LDR
; 汇编器自动在附近创建"文字池"（literal pool）存储值

; 加载大常量
ldr x0, =0x1234567890ABCDEF    ; 汇编器生成 .quad + LDR [PC, #offset]

; 加载符号地址
ldr x0, =message               ; 加载 message 的地址

; 加载字符串
ldr w0, ='FIXT'                ; 加载4字节字符串常量
```

### 条件汇编

```asm
.equ DEBUG, 1

.if DEBUG
    ; 调试代码
    mov x0, #'D'
    bl print_char
.else
    ; 生产代码
.endif

; 条件汇编在汇编阶段决定，不影响运行时
```

## 与 C 的对照

```c
// C 的数据定义
int counter = 0;                    // → .data: .quad 0
const char *message = "Hello";      // → .rodata: .asciz "Hello"
int big_array[1024];                // → .bss: .space 4096
#define BUF_SIZE 256               // → .equ BUF_SIZE, 256
int table[] = {10, 20, 30, 40, 50}; // → .word 10, 20, 30, 40, 50
```

## 常见错误

1. **.align 参数混淆**：AArch64 用 2^n，x86 用 n。跨架构用 .balign。
2. **.string vs .ascii**：.string 自动加 NULL，.ascii 不加。需要 NULL 结尾的字符串用 .string。
3. **BSS 段写非零初始值**：BSS 应全零，写非零值会被忽略。

## HFT 关联

- `.align` 控制 cache line 对齐 → HFT 数据结构需 64 字节对齐避免 false sharing
- `.bss` 的零初始化数据不占二进制空间 → 减少加载时间
- 理解段概念是链接脚本（Ch9）的基础

```asm
; HFT：避免 false sharing 的数据对齐
.section .data
.align 6                          ; 64 字节对齐（cache line）
per_core_counter:
    .quad 0
    .space 56                     ; padding 到 64 字节
next_core_counter:
    .quad 0
    .space 56
```

## 自测题

1. `.align 4` 在 AArch64 上对齐多少字节？
<details><summary>答案</summary>
2^4 = 16 字节。注意与 x86 不同——x86 的 `.align 4` 是 4 字节对齐。AArch64 的参数是 2 的幂指数。
</details>

2. `.string` 和 `.ascii` 的区别？
<details><summary>答案</summary>
`.string "Hello"` 存储 6 字节（5字符 + NULL）。`.ascii "Hello"` 存储 5 字节（无 NULL）。
</details>

3. 以下代码中 `counter` 的地址对齐到多少？
```asm
.data
msg:
    .string "Hi"    // 3 字节
counter:
    .quad 0
```
<details><summary>答案</summary>
`counter` 紧跟 `msg` 后，地址为 msg+3，未对齐到 8 字节。应加 `.align 3` 再定义 `counter`。
</details>

4. 为什么推荐用 `.balign` 而非 `.align`？
<details><summary>答案</summary>
`.balign n` 在所有架构上都表示对齐到 n 字节，语义统一。`.align n` 在 AArch64 上是 2^n 字节，在 x86 上是 n 字节，跨架构代码容易出错。`.balign` 避免了这种歧义。
</details>

5. `.equ` 定义的常量和 `.quad` 定义的数据有什么区别？
<details><summary>答案</summary>
- `.equ NAME, VALUE`：汇编期常量，不占用任何内存空间，仅在汇编时替换。类似 C 的 `#define`。
- `.quad VALUE`：在当前段中写入 8 字节数据，占用内存。类似 C 的全局变量定义。
- `.equ` 用于常量（如地址、大小），`.quad` 用于初始化数据。
</details>

## 参考与延伸

- 原书 §8.2
- [8.3 段](03-sections.md)
- [Ch9 链接脚本](../../chapter-09-linker-scripts/notes/section-0-本章完整概述.md)
