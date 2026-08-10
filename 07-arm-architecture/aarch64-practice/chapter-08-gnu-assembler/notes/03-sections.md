# 8.3 段（Section）

> 来源：§8.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

.text/.data/.bss 段的概念、段属性、自定义段的使用，以及链接器如何合并段。

## 核心要点

### 标准段

| 段 | 内容 | 可读 | 可写 | 可执行 | 占二进制 |
|----|------|------|------|--------|----------|
| .text | 代码 | ✓ | ✗ | ✓ | 是 |
| .data | 已初始化数据 | ✓ | ✓ | ✗ | 是 |
| .bss | 未初始化数据 | ✓ | ✓ | ✗ | 否（零填充） |
| .rodata | 只读数据 | ✓ | ✗ | ✗ | 是 |

### 段的使用

```asm
.section .text           ; 切换到代码段
.global _start
_start:
    mov x0, #1
    bl my_func

.section .data           ; 切换到数据段
counter:
    .quad 0              ; 已初始化的全局变量

.section .rodata         ; 切换到只读数据段
msg:
    .asciz "Hello"       ; 字符串常量

.section .bss            ; 切换到 BSS 段
buffer:
    .space 256           ; 未初始化的缓冲区
```

### 简写形式

```asm
.text                    ; 等价于 .section .text
.data                    ; 等价于 .section .data
.bss                     ; 等价于 .section .bss
```

### 自定义段

```asm
; 自定义段名和属性
.section .mydata, "aw"   ; a=allocatable, w=writable
custom_var:
    .quad 0x1234

; 段属性字符：
; a = allocatable（可分配内存）
; w = writable（可写）
; x = executable（可执行）
; 不写 = 默认属性（只读不可执行）
```

### 段属性详解

```
.text → 属性 "ax"（allocatable + executable）→ 只读可执行
.data → 属性 "aw"（allocatable + writable）→ 可读可写不可执行
.bss  → 属性 "aw"（同 .data，但不占文件空间）
.rodata → 属性 "a"（allocatable，不可写不可执行）→ 只读
```

### BSS 段的特殊性

```
.bss 段为什么不需要文件空间？

.bss 中所有变量初始化为 0：
  - 二进制文件只记录 .bss 的起始地址和大小
  - 程序加载时 OS 零填充该区域
  - 不需要在文件中存储大量零值

对比 .data：
  - .data 中的变量有非零初始值
  - 必须在文件中存储初始值
  - 加载时从文件拷贝到内存

示例：
  int counter = 42;     → .data（需要文件存储 42）
  int counter = 0;      → .bss（不需要文件，加载时清零）
  int counter;          → .bss（未初始化默认 0）
```

### 链接器如何合并段

```
文件 A:                    文件 B:                    链接后：
  .text                     .text                     .text
    func_a                    func_b                    func_a
  .data                     .data                       func_b
    var_a                     var_b                   .data
  .bss                      .bss                        var_a
    buf_a                     buf_b                     var_b
                                                       .bss
                                                         buf_a
                                                         buf_b

链接器把所有文件的 .text 合并、.data 合并、.bss 合并。
段在内存中的位置由链接脚本（Ch9）决定。
```

### 内核特殊段

```asm
; Linux 内核使用自定义段实现特殊功能

; __init 段：初始化后释放
.section .init.text, "ax"
early_init:
    ; 只在启动时调用
    ; start_kernel 完成后，free_initmem() 释放此段

; __percpu 段：每 CPU 独立数据
.section .data..percpu, "aw"
    .quad 0              ; 每个 CPU 有一份独立副本

; __exception 段：异常向量表
.section .exception.text, "ax"
    ; 异常处理代码
```

## 与 C 的对照

```c
// C 代码的段分布
int global_var = 42;           // → .data
int zero_var = 0;              // → .bss（优化）
int uninit_var;                // → .bss
const char *str = "Hello";     // "Hello" → .rodata, str → .data
const int const_val = 100;     // → .rodata
static int static_var = 1;    // → .data

void func() { ... }            // → .text

// GCC 属性指定段
int hot_data __attribute__((section(".hotdata"))) = 0;
```

## 常见错误

1. **BSS 写非零初始值**：`.bss` 段的变量应为零，写非零值行为未定义。
2. **代码段写数据**：`.text` 段只读，运行时写入 → 页保护异常。
3. **自定义段忘记属性**：`.section .mydata` 不写属性 → 默认只读，写入异常。

## HFT 关联

- .text 标记只读+可执行 → 防止意外写入
- .data 标记可写不可执行 → 防止代码注入
- 内核 `__init` 段在启动后释放 → 节省内存
- 自定义段可实现 cache line 隔离 → 避免 false sharing

```asm
; HFT：per-core 数据放在独立段，避免 false sharing
.section .data.percpu, "aw"
.align 6                          ; 64 字节对齐（cache line）
core0_stats:
    .quad 0, 0, 0, 0              ; 32 字节数据
    .space 32                     ; padding 到 64 字节
core1_stats:
    .quad 0, 0, 0, 0
    .space 32
```

## 自测题

1. .bss 段为什么不占二进制文件空间？
<details><summary>答案</summary>
.bss 存储未初始化的变量，值全为 0。程序加载时由 OS 零填充，不需要在二进制中存储零值。只记录起始地址和大小即可。
</details>

2. 自定义段 `.section .mydata, "aw"` 的属性是什么？
<details><summary>答案</summary>
`a`=allocatable，`w`=writable。没有 `x`（不可执行）。可读可写但不可执行，适合存放数据。
</details>

3. 内核的 `__init` 段有什么特殊用途？
<details><summary>答案</summary>
存放只在初始化时调用的函数。内核启动完成后释放这部分内存（free_initmem），节省运行时内存。
</details>

4. C 中的 `int x = 0;` 会放在 .bss 还是 .data？
<details><summary>答案</summary>
通常放在 .bss。GCC 会优化：初始值为 0 的全局变量放入 .bss 段，不占文件空间。但如果用了 `__attribute__((section(".data")))` 强制指定段，则放入 .data。未初始化的全局变量 `int x;` 默认也在 .bss（C 标准要求静态存储期变量初始化为 0）。
</details>

5. 为什么 .text 段不能在运行时写入？
<details><summary>答案</summary>
.text 段的页表属性是只读+可执行（RX）。如果运行时写入 .text → 触发数据中止异常（对齐/权限错误）。这是操作系统的安全机制（W^X 原则：同一段内存不能同时可写和可执行），防止代码注入攻击。自修改代码需要先 mprotect 改为可写，修改后改回可执行。
</details>

## 参考与延伸

- 原书 §8.3
- [8.2 伪指令](02-directives.md)
- [Ch9 链接脚本段布局](../../chapter-09-linker-scripts/notes/section-0-本章完整概述.md)
