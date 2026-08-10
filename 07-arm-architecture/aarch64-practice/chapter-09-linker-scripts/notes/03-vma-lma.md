# 9.3 VMA vs LMA

> 来源：§9.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

VMA（Virtual Memory Address，虚拟内存地址）和 LMA（Load Memory Address，加载内存地址）的区别、`AT()` 指令的用法、以及内核/裸机中 VMA≠LMA 的典型场景。

## 概念定义

| 概念 | 全称 | 含义 | 何时使用 |
|------|------|------|---------|
| VMA | Virtual Memory Address | 运行时地址，CPU 执行时看到的地址 | 程序运行时的所有访存 |
| LMA | Load Memory Address | 加载地址，程序被加载器放到物理内存的地址 | 程序加载/启动时 |

简单场景下 **VMA = LMA**（普通 Linux 用户程序），但在以下场景 **VMA ≠ LMA**：

| 场景 | VMA | LMA | 说明 |
|------|-----|-----|------|
| Linux 内核 | 0xFFFF...（高位虚拟地址） | 物理低位（0x80000） | 内核被加载到物理低位，运行在虚拟高位 |
| 嵌入式 Flash→RAM | RAM 地址 | Flash 地址 | 代码存在 Flash，启动后拷贝到 RAM 执行 |
| XIP（eXecute In Place） | Flash 地址 | Flash 地址 | 直接在 Flash 中执行，VMA=LMA=Flash |

## AT 指令

```ld
/* 语法 */
段名 VMA地址 : AT(LMA地址) { ... }

/* 实例 */
.text 0x80000 : AT(0x40000) { *(.text) }
/*  VMA=0x80000       LMA=0x40000  */
```

| 用法 | 含义 |
|------|------|
| `.text 0x80000 : AT(0x40000)` | VMA=0x80000, LMA=0x40000 |
| `.text 0x80000 :` | VMA=0x80000, LMA=0x80000（默认 LMA=VMA） |
| `.text : AT(LOADADDR(.text) + 0x1000)` | LMA = 同段 VMA + 偏移 |

## readelf 验证 VMA 和 LMA

```bash
# readelf -l 显示 Program Headers
readelf -l vmlinux

# 输出示例：
# Program Headers:
#   Type      Offset    VirtAddr(VMA)  PhysAddr(LMA)  FileSiz  MemSiz
#   LOAD      0x000000  0xffff0000...  0x00080000     0x100000 0x200000
#
# VirtAddr ≠ PhysAddr → VMA ≠ LMA
```

## 内核启动流程（VMA≠LMA 的经典案例）

```
1. Bootloader（U-Boot）加载内核镜像到物理地址 0x80000
   → 此刻 LMA=0x80000

2. CPU 从 0x80000 开始执行
   → 此时 MMU 未开启，VMA=LMA=0x80000（恒等映射）

3. 内核 startup 代码建立页表
   → 内核虚拟地址 0xFFFF000000000000

4. 开启 MMU
   → CPU 从 0x80000 切换到 0xFFFF0000000080000 执行
   → VMA=0xFFFF..., LMA=0x80000

5. 启动代码拷贝 .data 段
   → .data 的 LMA 在物理低位，但 VMA 在虚拟高位
   → 需要把 .data 从 LMA 拷贝到 VMA
   → 否则 CPU 用 VMA 访问会 page fault
```

### 链接脚本中的体现

```ld
/* 简化的内核链接脚本 */
SECTIONS
{
    . = 0xFFFF000000000000;     /* VMA 起始（高位虚拟地址） */

    .text : {
        _stext = .;
        *(.text)
        _etext = .;
    }

    . = ALIGN(4096);
    .data : AT(ADDR(.data) - 0xFFFF000000000000)  /* LMA = VMA - 偏移 */
    {
        _sdata = .;
        *(.data)
        _edata = .;
    }
}
```

## 嵌入式 Flash→RAM 场景

```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    /* 代码存在 Flash，启动后拷贝到 RAM */
    .text : {
        *(.text)
    } > RAM AT> FLASH    /* VMA 在 RAM，LMA 在 Flash */

    /* 数据存在 Flash，启动时拷贝到 RAM */
    .data : {
        *(.data)
    } > RAM AT> FLASH

    /* BSS 只在 RAM，不需要 Flash */
    .bss : {
        *(.bss)
    } > RAM
}
```

启动代码需要：
```c
/* 从 LMA(Flash) 拷贝 .data 到 VMA(RAM) */
extern char _sdata, _edata, _sidata;
memcpy(&_sdata, &_sidata, &_edata - &_sdata);

/* .bss 清零 */
extern char _sbss, _ebss;
memset(&_sbss, 0, &_ebss - &_sbss);
```

## 常用地址相关函数

| 函数 | 用途 |
|------|------|
| `ADDR(.text)` | 段的 VMA |
| `LOADADDR(.text)` | 段的 LMA |
| `SIZEOF(.text)` | 段大小 |
| `ALIGN(align)` | 对齐 |
| `DEFINED(sym)` | 判断符号是否已定义 |

```ld
__data_start  = ADDR(.data);         /* VMA */
__data_load   = LOADADDR(.data);     /* LMA */
__data_size   = SIZEOF(.data);       /* 大小 */
```

## HFT 关联

- **内核被加载到物理低位（LMA），运行在高位虚拟地址（VMA）** → 内核启动分析必须理解 VMA/LMA
- **BenOS 启动时 VMA=LMA（恒等映射），开 MMU 后切换** → 裸机实验中先恒等映射再切换
- **SRAM vs DRAM 布局** → 热数据放 SRAM（低延迟），冷数据放 DRAM，用 MEMORY 块区分
- **精确地址控制** → HFT 中某些数据结构需放在固定地址（如 MMIO 寄存器映射）

## 自测题

1. VMA 和 LMA 什么时候不同？
<details><summary>答案</summary>
（1）内核启动：LMA=物理低位（0x80000），VMA=虚拟高位（0xFFFF...）（2）嵌入式 Flash→RAM：LMA=Flash 地址，VMA=RAM 地址（3）DSP 的 L1 SRAM→L2 DDR 拷贝。简单 Linux 用户程序 VMA=LMA。
</details>

2. 内核启动为什么需要从 LMA 拷贝 .data 到 VMA？
<details><summary>答案</summary>
开 MMU 后 CPU 用 VMA（虚拟高位地址）访问数据，但 .data 的实际内容存在 LMA（物理低位）。如果不拷贝，CPU 访问 VMA 地址时找不到数据 → page fault。启动代码必须在开 MMU 前或紧接其后做拷贝。
</details>

3. `AT()` 的作用是什么？不指定 AT 时 LMA 是多少？
<details><summary>答案</summary>
`AT(addr)` 指定段的 LMA（加载地址）。不指定时默认 LMA=VMA。`AT> 区域名` 则把 LMA 放到指定 MEMORY 区域。
</details>

4. 如何用 `readelf` 验证某段的 VMA 和 LMA 是否相同？
<details><summary>答案</summary>
`readelf -l 文件名` 查看 Program Headers 表。比较 VirtAddr 列（VMA）和 PhysAddr 列（LMA）。两者相同 → VMA=LMA；不同 → VMA≠LMA。
</details>

5. 嵌入式 Flash→RAM 场景中，.bss 段需要从 Flash 拷贝吗？
<details><summary>答案</summary>
不需要。.bss 是未初始化数据，不占 Flash 存储空间（链接器在 ELF 中只记录大小）。启动代码只需把 RAM 中的 .bss 区域清零即可（memset），不需要从 Flash 拷贝。
</details>

## 参考与延伸

- 原书 §9.3
- [9.4 常用链接器选项](04-linker-options.md)
- [Ch14 MMU 地址映射](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
