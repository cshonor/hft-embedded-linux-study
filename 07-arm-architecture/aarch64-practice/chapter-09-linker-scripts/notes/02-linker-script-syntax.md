# 9.2 链接脚本语法

> 来源：§9.2 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

链接脚本（Linker Script, `.ld` 文件）的基本语法：ENTRY 指令、SECTIONS 块、位置计数器 `.`、段定义、MEMORY 块、ALIGN 对齐。

## 链接脚本骨架

```ld
/* 最小链接脚本 */
ENTRY(_start)          /* 程序入口符号 */

SECTIONS
{
    . = 0x40000000;    /* 设置起始地址（位置计数器赋值） */

    .text : {          /* 输出段 .text */
        *(.text)       /* 匹配所有输入文件的 .text 段 */
    }

    . = ALIGN(8);      /* 对齐到 8 字节 */
    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
        *(COMMON)      /* 公共符号 */
    }
}
```

## 核心语法元素

### 1. ENTRY 指令

```ld
ENTRY(symbol)
```

指定程序入口地址。程序加载后 CPU 的 PC 被设为此符号的地址。

- 不指定时默认取 `.text` 段起始地址
- 裸机开发通常 `ENTRY(_start)`，其中 `_start` 是 startup 汇编里的入口
- Linux 内核 `ENTRY(stext)`

### 2. 位置计数器 `.`

| 用法 | 含义 |
|------|------|
| `. = 0x80000;` | 设置当前位置 = 指定地址 |
| `. = . + 0x1000;` | 跳过 4KB（留空洞） |
| `. = ALIGN(8);` | 对齐到 8 字节边界 |
| `_start = .;` | 记录当前位置到符号 |

`.` 是一个变量，表示**当前输出段的虚拟地址（VMA）**。每放入一段数据它就增加相应大小。

### 3. 输出段定义

```ld
/* 完整格式 */
段名 VMA地址 : AT(LMA地址) {
    /* 输入段匹配 */
    文件名(段名)
    *(段名)           /* 通配：所有文件 */
    *(.text.*)        /* 通配：.text.init .text.hot 等 */

    /* 保留符号 */
    _text_start = .;
    *(.text)
    _text_end = .;
} > 内存区域 :PHDR
```

### 4. 输入段匹配

| 模式 | 含义 |
|------|------|
| `*(.text)` | 所有文件的 .text 段 |
| `obj.o(.text)` | 只有 obj.o 的 .text 段 |
| `*(.text .text.*)` | .text 及所有 .text.* 子段 |
| `*(EXCLUDE_FILE (*crt*.o) .text)` | 排除特定文件 |

### 5. ALIGN 对齐

```ld
. = ALIGN(4);          /* 对齐到 4 字节 */
. = ALIGN(4096);       /* 对齐到 4KB 页 */
. = ALIGN(8) + 0x100;  /* 对齐后偏移 */
```

对齐在链接脚本中至关重要：
- **AAPCS64 要求 SP 16 字节对齐** → 栈段起始地址必须对齐
- **页表映射以 4KB 为单位** → 各段起始地址需 4KB 对齐
- **DMA 需要 cache line 对齐** → 通常 64 字节

### 6. MEMORY 块

```ld
MEMORY
{
    RAM (rwx)  : ORIGIN = 0x80000, LENGTH = 256M
    ROM (rx)   : ORIGIN = 0x00000, LENGTH = 64M
    SRAM(rwx)  : ORIGIN = 0xFF000000, LENGTH = 64K
}

SECTIONS
{
    .text : { *(.text) }  > ROM     /* 代码放 ROM */
    .data : { *(.data) }  > RAM AT(ROM)  /* 数据放 RAM，从 ROM 加载 */
    .bss  : { *(.bss) }  > RAM
}
```

| 字段 | 含义 |
|------|------|
| `ORIGIN` (或 `org`, `o`) | 起始地址 |
| `LENGTH` (或 `len`, `l`) | 大小 |
| 属性 `rwx` | 读/写/执行权限标记 |

### 7. PHDRS 段头表

```ld
PHDRS
{
    headers PT_PHDR PHDRS;
    text    PT_LOAD FILEHDR PHDRS;
    data    PT_LOAD;
}
```

| 字段 | 含义 |
|------|------|
| `PT_LOAD` | 可加载段 |
| `PT_DYNAMIC` | 动态链接信息 |
| `PT_INTERP` | 解释器路径 |

## 常用链接脚本命令

| 命令 | 用途 |
|------|------|
| `ENTRY(sym)` | 设入口符号 |
| `OUTPUT_FORMAT(elf64-littleaarch64)` | 输出格式 |
| `OUTPUT_ARCH(aarch64)` | 目标架构 |
| `PROVIDE(sym = .)` | 定义符号（仅当未被定义时） |
| `ASSERT(expr, "msg")` | 断言检查 |
| `KEEP(*(.init))` | 防止 --gc-sections 删除 |

```ld
/* 实际例子 */
PROVIDE(_stack_top = ORIGIN(RAM) + LENGTH(RAM));
ASSERT(_end <= ORIGIN(RAM) + LENGTH(RAM), "RAM overflow!");
```

## HFT 关联

- **代码段放在特定地址** → 配合 MMU 页表映射，热路径放 SRAM（低延迟）
- **数据段对齐到 cache line** → `ALIGN(64)` 减少 false sharing
- **`KEEP()` 保留 init 段** → 防止 `--gc-sections` 误删启动代码
- **MEMORY 块区分 SRAM/DRAM** → 热数据放 SRAM、冷数据放 DRAM

## 自测题

1. 链接脚本中 `.` 代表什么？
<details><summary>答案</summary>
位置计数器（Location Counter），表示当前的虚拟地址（VMA）。链接器从起始地址开始，每放入一段数据 `.` 就增加相应大小。可以赋值、读取、对齐。
</details>

2. `*(.text)` 和 `obj.o(.text)` 的区别？
<details><summary>答案</summary>
`*(.text)` 是通配模式，匹配**所有输入文件**的 .text 段。`obj.o(.text)` 只匹配 **obj.o 文件**的 .text 段。通配模式常用于通用段，精确模式用于排序或特殊处理。
</details>

3. ENTRY(_start) 的作用是什么？不写会怎样？
<details><summary>答案</summary>
指定程序入口地址为 `_start` 符号。程序加载后 CPU 从此地址开始执行。不指定时默认取 `.text` 段的起始地址，但裸机开发中 .text 起始可能不是实际入口（可能有头部数据），所以必须显式指定。
</details>

4. `PROVIDE(symbol = expr)` 和直接 `symbol = expr` 有什么区别？
<details><summary>答案</summary>
`PROVIDE` 只在符号未被定义时才定义它（条件定义）。如果源码或目标文件已经定义了该符号，PROVIDE 不覆盖。直接赋值 `symbol = expr` 是无条件定义，会覆盖已有定义。
</details>

5. `KEEP(*(.init))` 的作用是什么？
<details><summary>答案</summary>
防止 `--gc-sections` 删除 .init 段。链接器的 GC 算法从入口符号可达性分析删除不可达段，.init 等启动段可能不被入口直接引用，KEEP 强制保留。
</details>

## 参考与延伸

- 原书 §9.2
- [9.3 VMA vs LMA](03-vma-lma.md)
- GNU ld 文档：https://sourceware.org/binutils/docs/ld/Scripts.html
