# Ch9 完整总结 · 链接器与链接脚本

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

链接器 `ld` 把多个 `.o` 文件合并成可执行文件/镜像，按**链接脚本**排列各段在内存中的位置。理解 VMA/LMA 是读 U-Boot、内核链接脚本的基础。

---

## 9.1 链接基本概念

| 概念 | 含义 |
|------|------|
| **VMA** (Virtual Memory Address) | 程序运行时的虚拟地址 |
| **LMA** (Load Memory Address) | 程序加载时的物理地址 |
| **入口点** (Entry Point) | 程序执行的第一条指令地址 |
| **位置计数器** `.` | 链接脚本中的当前地址指针 |

> VMA ≠ LMA 的情况：内核启动时代码从物理地址加载（LMA），但运行在虚拟地址（VMA），需要重定位。

---

## 9.2 链接脚本语法

```lds
ENTRY(_start)              /* 入口点 */

SECTIONS
{
    . = 0x40000000;         /* 设置起始地址（位置计数器） */

    .text : {
        *(.text)            /* 所有输入文件的 .text 段 */
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
        . = ALIGN(8);
    }
}
```

### 关键语法

| 语法 | 作用 |
|------|------|
| `ENTRY(sym)` | 设置入口点 |
| `. = addr` | 设置位置计数器 |
| `ALIGN(n)` | 对齐到 n 字节 |
| `*(.section)` | 匹配所有输入文件的该段 |
| `obj.o(.section)` | 匹配特定文件的该段 |
| `AT(addr)` | 设置 LMA（加载地址≠运行地址） |

---

## 9.3 VMA vs LMA ⭐

```lds
/* BenOS 示例：代码加载到 0x60000000，运行也在 0x60000000 */
. = 0x60000000;
.text : { *(.text) }

/* 内核示例：加载在物理地址，运行在虚拟地址 */
. = 0xFFFF000000000000;        /* VMA = 高位虚拟地址 */
.text : AT(0x80000000) {       /* LMA = 物理地址 */
    *(.text)
}
```

> **什么时候 VMA ≠ LMA？**
> - Linux 内核：LMA=物理地址，VMA=内核虚拟地址（0xFFFF...开头）
> - U-Boot SPL：LMA=SRAM 地址，VMA 可能不同
> - BenOS 恒等映射时：VMA=LMA

---

## 9.4 常用链接器选项

| 选项 | 作用 |
|------|------|
| `-T script.lds` | 指定链接脚本 |
| `-o output` | 输出文件名 |
| `-Map mapfile` | 生成地址映射表 |
| `--gc-sections` | 删除未使用的段 |
| `-Ttext addr` | 快速设置 .text 段地址 |

---

## 9.5 分析链接结果

```bash
# 查看段布局
aarch64-linux-gnu-objdump -h benos.elf

# 查看符号表
aarch64-linux-gnu-nm benos.elf

# 反汇编
aarch64-linux-gnu-objdump -d benos.elf

# 查看入口点
aarch64-linux-gnu-readelf -h benos.elf
```

### objdump -h 输出示例

```
Idx Name      Size      VMA               LMA
  0 .text     00001000  0000000060000000  0000000060000000
  1 .rodata   00000020  0000000060001000  0000000060001000
  2 .data     00000010  0000000060002000  0000000060002000
  3 .bss      00000008  0000000060002010  0000000060002010
```

> VMA=LMA → 恒等映射场景。如果 VMA≠LMA，需要启动代码做重定位。

---

## 9.6 Linux 内核链接脚本分析

`arch/arm64/kernel/vmlinux.lds.S` 关键结构：

```lds
/* 内核起始虚拟地址 */
. = KIMAGE_VADDR;     /* 通常是 0xFFFF800000000000 */

.text : {
    _stext = .;       /* 代码段起始 */
    *(.text)
    _etext = .;       /* 代码段结束 */
}

/* __init 段：启动后释放 */
.init.text : {
    __init_start = .;
    *(.init.text)
    __init_end = .;
}
```

**关键符号：**

| 符号 | 含义 |
|------|------|
| `_stext` / `_etext` | 代码段起止 |
| `_sdata` / `_edata` | 数据段起止 |
| `__bss_start` / `__bss_stop` | BSS 段起止 |
| `__init_start` / `__init_end` | init 段（启动后可释放） |

---

## 9.7 实验要点

| 实验 | 内容 | 平台 |
|------|------|------|
| 9-1 | 分析链接脚本文件 | QEMU |
| 9-2 | 输出每个段的内存布局 | QEMU |
| 9-3 | 加载地址不等于运行地址（VMA≠LMA） | QEMU |
| 9-4 | 分析 Linux 5.0 内核的链接脚本文件 | QEMU |

---

## 9.8 易错点清单

1. **忘记 ENTRY()** → 链接器可能选错入口点，程序从错误地址执行。
2. **.bss 没有对齐** → 运行时访问 BSS 变量可能触发对齐异常。
3. **VMA≠LMA 没做重定位** → 代码加载在 LMA，但运行在 VMA，不做重定位会跳到错误地址。
4. **忘记 ALIGN** → 段间可能有间隙，或地址不对齐导致异常。
5. **链接脚本用错架构** → ARM32 和 AArch64 的链接脚本格式有差异。

---

## 书中思考题（自测）

1. VMA 和 LMA 的区别？什么时候需要 VMA ≠ LMA？
2. 链接脚本中的 `.` 代表什么？
3. 如何查看 ELF 文件的段布局？用什么工具？
4. Linux 内核为什么把 .init 段单独放？启动后怎么处理？
5. `AT()` 关键字在链接脚本中的作用？

**参考答案：**

1. VMA=运行时虚拟地址，LMA=加载时物理地址。内核/重定位场景需要 VMA≠LMA。  
2. **位置计数器**（当前地址指针）。  
3. `objdump -h` 看段布局；`nm` 看符号；`readelf -h` 看入口点。  
4. .init 段只在启动时用（初始化函数）；启动后 `free_initmem()` 释放回收。  
5. 设置段的 **LMA**（加载地址），让 LMA 可以不同于 VMA。

---

上一章 [Ch8 GNU汇编器](../../chapter-08-gnu-assembler/) · 下一章 [Ch10 GCC内联汇编](../../chapter-10-gcc-inline-asm/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
