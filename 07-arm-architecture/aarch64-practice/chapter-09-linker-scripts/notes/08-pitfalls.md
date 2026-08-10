# 9.8 易错点清单

> 来源：§9.8 · 精读 · [章总览](section-0-本章完整概述.md)

## 8 大易错点

### 1. VMA/LMA 混淆

**症状**：启动后数据访问错误、page fault、读到全零。

**原因**：VMA≠LMA 但启动代码没拷贝 .data 从 LMA 到 VMA。CPU 用 VMA 访问数据，但数据实际在 LMA 地址。

**修复**：启动代码中加拷贝：

```c
/* 正确做法 */
extern char _data_start, _data_end, _data_load;
memcpy(&_data_start, &_data_load, &_data_end - &_data_start);

/* _data_load = LOADADDR(.data) → LMA */
/* _data_start = ADDR(.data)    → VMA */
```

### 2. 位置计数器忘记对齐

**症状**：段不对齐、AAPCS64 要求 16 字节 SP 对齐导致 crash、页表映射不对齐。

**原因**：链接脚本中段间没有 `. = ALIGN(n)`。

```ld
/* ✗ 错误：.data 紧跟 .text，可能不是 8 字节对齐 */
.text : { *(.text) }
.data : { *(.data) }

/* ✓ 正确：对齐后再放 */
.text : { *(.text) }
. = ALIGN(8);
.data : { *(.data) }
. = ALIGN(8);
.bss : { *(.bss) }
```

| 对齐值 | 用途 |
|------|------|
| `ALIGN(4)` | 基本 4 字节（32 位） |
| `ALIGN(8)` | 64 位数据（double/long） |
| `ALIGN(16)` | AAPCS64 SP 对齐 |
| `ALIGN(64)` | cache line |
| `ALIGN(4096)` | 页边界（MMU 映射） |

### 3. ENTRY 指定错误

**症状**：程序入口地址不对、执行了垃圾指令、立即 crash。

**原因**：
- ENTRY 指定的符号不存在 → 链接器用默认入口（可能指向错误地址）
- ENTRY 指定了 C 函数但没用 `extern "C"`（C++ name mangling）
- _start 在 .text.boot 但被 --gc-sections 删了

```ld
/* ✓ 用 KEEP 防止 GC 误删 */
.text.boot : {
    KEEP(*(.text.boot))   /* _start 在这里 */
}
ENTRY(_start)
```

### 4. 段属性遗漏

**症状**：代码段可写（安全漏洞）、数据段可执行（安全漏洞）、运行时 segfault。

**原因**：链接脚本或 MEMORY 块没设权限。

```ld
/* ✓ MEMORY 块设权限 */
MEMORY
{
    ROM (rx)  : ORIGIN = 0x0, LENGTH = 256K    /* 只读+执行 */
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 64K  /* 读写执行 */
}

/* ✓ SECTIONS 指定段到对应区域 */
.text  : { *(.text) }   > ROM
.data  : { *(.data) }   > RAM AT> ROM
.bss   : { *(.bss) }   > RAM
```

| 权限 | 允许操作 | 典型段 |
|------|---------|-------|
| R | 读 | .text, .rodata, .data, .bss |
| W | 写 | .data, .bss |
| X | 执行 | .text |

**安全规则**：.text 不可写（防 shellcode 注入），.data/.bss 不可执行（防代码注入）。

### 5. .bss 没清零

**症状**：全局变量初值不是 0、行为不确定。

**原因**：C 标准规定 .bss 段（未初始化全局变量）初值为 0，但裸机无 OS 做这件事，需启动代码手动清零。

```asm
/* ✓ 启动代码清零 .bss */
adr_l x0, _bss_start
adr_l x1, _bss_end
1:  cmp x0, x1
    b.hs 2f
    str xzr, [x0], #8     /* 写 0 */
    b 1b
2:
```

### 6. 段重叠

**症状**：链接报错或运行时数据覆盖。

**原因**：手动指定地址时计算错误。

```ld
/* ✗ 错误：.data 地址可能和 .text 重叠 */
.text 0x80000 : { *(.text) }    /* .text 占 0x80000-0x80100 */
.data 0x80008 : { *(.data) }    /* .data 在 .text 中间！ */

/* ✓ 正确：用位置计数器自动计算 */
. = 0x80000;
.text : { *(.text) }
. = ALIGN(8);
.data : { *(.data) }
```

### 7. --gc-sections 误删启动代码

**症状**：链接后找不到 _start 或启动函数。

**原因**：GC 可达性分析没覆盖到某些启动代码。

```ld
/* ✓ 用 KEEP 保护 */
.init : {
    KEEP(*(.text.boot))
    KEEP(*(.init))
}
```

### 8. 忘记导出符号给 C 代码

**症状**：C 代码引用链接脚本中定义的符号报 `undefined reference`。

**原因**：链接脚本中的符号需要 C 代码声明为 `extern`。

```ld
/* 链接脚本 */
_text_end = .;
_stack_top = ORIGIN(RAM) + LENGTH(RAM);
```

```c
/* C 代码 */
extern char _text_end;     /* 不加取地址，用 & 取地址 */
extern char _stack_top;

void *get_stack_top(void) {
    return &_stack_top;     /* 必须取地址！符号的"值"就是地址 */
}
```

> **关键**：链接脚本中的符号本质是地址标签，C 代码中声明为 `extern char` 然后取地址 `&symbol`。

## 易错点速查表

| # | 易错点 | 症状 | 一句话修复 |
|---|--------|------|-----------|
| 1 | VMA/LMA 混淆 | 数据访问错误 | 启动代码拷贝 .data |
| 2 | 忘记对齐 | crash/性能差 | `ALIGN(8)`/`ALIGN(16)` |
| 3 | ENTRY 错误 | 入口不对 | `ENTRY(_start)` + `KEEP` |
| 4 | 段属性遗漏 | 安全漏洞 | MEMORY 块设 rwx |
| 5 | .bss 没清零 | 变量非 0 | 启动代码 memset |
| 6 | 段重叠 | 数据覆盖 | 用位置计数器 |
| 7 | GC 误删代码 | 找不到函数 | `KEEP()` |
| 8 | 符号未导出 | 链接报错 | C 中 `extern` + 取地址 |

## 自测题

1. 程序启动后立即 crash，可能是什么链接问题？
<details><summary>答案</summary>
（1）VMA≠LMA 但启动代码没拷贝 .data（2）ENTRY 指定的符号不存在或地址错误（3）.text 段没有可执行权限（4）位置计数器设错导致段重叠（5）.bss 没清零导致全局变量初值异常（6）--gc-sections 误删了启动代码。
</details>

2. 以下链接脚本有什么问题？
```ld
.text 0x80000 : { *(.text) }
.data 0x80008 : { *(.data) }
```
<details><summary>答案</summary>
两个问题：（1）.data 地址 0x80008 可能和 .text 重叠（如果 .text > 8 字节）（2）0x80008 不是 8 字节对齐。应改用位置计数器 + ALIGN：`. = 0x80000; .text : { *(.text) } . = ALIGN(8); .data : { *(.data) }`。
</details>

3. 为什么不能让 .text 段可写？
<details><summary>答案</summary>
安全风险：可写代码段可被注入 shellcode。攻击者可以利用缓冲区溢出往 .text 写恶意代码然后执行。现代 OS 把 .text 标记为只读+可执行（R-X），MMU 级别保护。W^X 原则：一个页要么可写要么可执行，不能同时。
</details>

4. C 代码中如何正确使用链接脚本定义的 `_bss_start` 符号？
<details><summary>答案</summary>
```c
extern char _bss_start;  /* 声明为外部变量 */
/* 使用时取地址（符号的值=地址） */
char *p = &_bss_start;
```
不能直接读 `char c = _bss_start;`，因为符号本质上是一个地址标签，不是变量值。正确的理解是 `_bss_start` 的地址就是链接脚本中 `.` 的位置。
</details>

5. `--gc-sections` 误删了启动代码，有哪些修复方法？
<details><summary>答案</summary>
（1）在链接脚本中用 `KEEP(*(.text.boot))` 强制保留（2）确保 ENTRY 指向的符号可达这些代码（3）在启动代码中加 `__attribute__((used))` 标记（4）检查启动代码的引用链是否从 ENTRY 可达。
</details>

## 参考与延伸

- 原书 §9.8
- [9.3 VMA vs LMA](03-vma-lma.md)
- [9.2 链接脚本语法](02-linker-script-syntax.md)
