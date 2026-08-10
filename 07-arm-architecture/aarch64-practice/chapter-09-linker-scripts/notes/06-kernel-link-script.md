# 9.6 Linux 内核链接脚本分析

> 来源：§9.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

分析 Linux 内核链接脚本 `arch/arm64/kernel/vmlinux.lds`，理解内核内存布局、init 段、percpu 段等关键设计。

## 内核地址空间布局

ARM64 Linux 内核的虚拟地址空间：

```
┌─────────────────────────────────────────┐ 0xFFFFFFFFFFFFFFFF
│                                         │
│    内核空间 (TTBR1)                      │
│    VA bit[63:48] = 1                     │
│    0xFFFF000000000000 起                 │
│                                         │
│    .text  (内核代码)                     │
│    .rodata (只读数据)                    │
│    .init  (启动后释放)                   │
│    .data  (可读写数据)                   │
│    .bss                              │   │
│    .percpu (每核独立副本)                │
│                                         │
├─────────────────────────────────────────┤
│              [hole]                      │
├─────────────────────────────────────────┤
│                                         │
│    用户空间 (TTBR0)                      │
│    VA bit[63:48] = 0                    │
│                                         │
│    0x0000...0000 起                     │
│                                         │
└─────────────────────────────────────────┘ 0x0000000000000000
```

### 为什么是 0xFFFF000000000000？

| 条件 | 说明 |
|------|------|
| ARMv8 VA 位宽 | 可配置 39/48/52 位 |
| TTBR0 vs TTBR1 | VA bit[63:48] = 0 → TTBR0（用户），= 1 → TTBR1（内核） |
| 地址隔离 | 用户态和内核态地址空间完全隔离 |
| 安全 | 用户不能直接访问内核内存 |

## vmlinux.lds 关键段分析

### 1. 起始地址

```ld
/* arch/arm64/kernel/vmlinux.lds */
. = PAGE_OFFSET + TEXT_OFFSET;
/* PAGE_OFFSET = 0xFFFF000000000000（高位虚拟地址） */
/* TEXT_OFFSET  = 0x80000（内核在物理内存中的偏移） */
```

### 2. 代码段

```ld
_text = .;
.text : {
    _stext = .;        /* 代码段起始符号 */
    *(.text.head)      /* 启动专用代码 */
    *(.text)           /* 主要代码 */
    *(.text.hot)       /* 热路径代码 */
    *(.text.unlikely)  /* 冷路径代码 */
    _etext = .;        /* 代码段结束符号 */
} = 0xaa1a0000         /* 段填充值（非法指令，调试用） */
```

**设计要点**：
- `_text`/`_etext` 符号供内核其他代码查询代码段范围
- `.text.hot` 和 `.text.unlikely` 分开 → 热路径集中放，提高 icache 命中率
- 段填充值 `0xaa1a0000` = 非法指令，如果误跳到这里会触发异常

### 3. init 段

```ld
__init_begin = .;
.init : {
    *(.init.text)      /* __init 标记的函数 */
    *(.init.data)      /* __initdata 标记的数据 */
}
__init_end = .;
```

**init 段的意义**：
- `__init` 函数只在启动时调用一次（如设备初始化）
- 启动完成后 `free_initmem()` 释放这段内存
- 节省几十 MB 内存

```c
/* 内核中标记 init 函数 */
static int __init board_init(void) { ... }

/* __init 宏展开到 .init.text 段 */
#define __init __section(".init.text")
```

### 4. percpu 段

```ld
.percpu : {
    __per_cpu_start = .;
    *(.data..percpu)
    __per_cpu_end = .;
}
```

**percpu 实现机制**：

```
链接后：percpu 数据只有一份模板
  __per_cpu_start ──────────────┐
  │  per_cpu_var_A (4 bytes)    │
  │  per_cpu_var_B (8 bytes)    │
  __per_cpu_end ────────────────┘

运行时：每 CPU 创建一份副本
  CPU0 副本: [var_A=0] [var_B=0]
  CPU1 副本: [var_A=0] [var_B=0]
  CPU2 副本: [var_A=0] [var_B=0]

访问 per_cpu(var_A, cpu) =
  base_addr + offset_of(var_A) + cpu * percpu_section_size
```

| 优势 | 说明 |
|------|------|
| 无锁 | 每核独立副本，不需要原子操作 |
| Cache 友好 | 自己核的副本在 L1 cache 中 |
| HFT 关联 | 网卡队列统计、定时器计数等用 percpu |

### 5. data/bss 段

```ld
_data = .;
.data : { *(.data) }
_edata = .;

__bss_start = .;
.bss : { *(.bss) }
__bss_stop = .;
```

启动时清零 .bss：
```c
/* arch/arm64/kernel/head.S */
/* 清零 .bss 段 */
adr_l x0, __bss_start
adr_l x1, __bss_stop
1:  cmp x0, x1
    b.hs 2f
    str xzr, [x0], #8
    b 1b
2:
```

## 内核链接脚本中的特殊段

| 段名 | 用途 |
|------|------|
| `.text.hot` | GCC `-freorder-functions` 标记的热函数 |
| `.text.unlikely` | 冷路径函数 |
| `.init.text` | `__init` 启动后释放 |
| `.exit.text` | `__exit` 卸载时调用 |
| `.data..percpu` | 每核数据 |
| `.rodata` | 只读数据（字符串常量、const） |
| `.note` | ELF note（build-id 等） |
| `.debug_*` | 调试信息 |

## 内核启动内存初始化流程

```
1. Bootloader 加载内核到物理 0x80000
   → 此时 MMU 关闭，VMA=LMA=物理地址

2. head.S 执行
   → 设置 init page table（恒等映射）
   → 开启 MMU
   → 切换到虚拟高位地址

3. start_kernel() → setup_arch()
   → 解析 boot 命令行
   → 初始化内存管理

4. 清零 .bss 段
   → 从 __bss_start 到 __bss_stop

5. free_initmem()
   → 释放 __init_begin 到 __init_end
   → 回收 init 段内存
```

## HFT 关联

- **percpu 段 → 每核独立数据** → 避免跨核共享和锁竞争，HFT 低延迟关键
- **.text.hot 段** → 热路径函数集中，减少 icache miss
- **init 段释放** → 启动后回收内存，运行时减少内存压力
- **内核 VMA 隔离** → 用户态无法直接读内核数据，安全
- **自定义段 `__section()`** → HFT 可把热数据结构强制放特定段，配合 MMU 映射到 SRAM

## 自测题

1. 为什么内核 VMA 设为 0xFFFF000000000000？
<details><summary>答案</summary>
ARMv8 VA bit[63:48] 全 1 → 走 TTBR1（内核页表基址寄存器），全 0 → 走 TTBR0（用户页表基址寄存器）。高位虚拟地址让内核和用户地址空间完全隔离，用户态无法直接访问内核内存。
</details>

2. `__init_begin` 和 `__init_end` 有什么用？
<details><summary>答案</summary>
标记 init 段的起止地址。`__init` 标记的函数放在 `.init.text` 段，只在启动时调用。内核启动完成后 `free_initmem()` 用 `__init_begin` 和 `__init_end` 的地址范围释放这段内存，回收几十 MB 空间。
</details>

3. percpu 段如何实现每核独立数据？运行时如何访问？
<details><summary>答案</summary>
链接脚本把所有 percpu 变量放在一起（模板）。内核为每个 CPU 创建一份副本。运行时通过 `TPIDR_EL1` 寄存器保存当前 CPU 的 percpu 基地址，加上变量在模板中的偏移，得到当前 CPU 的副本地址。每核独立 → 无锁访问。
</details>

4. `.text.hot` 和 `.text.unlikely` 分开有什么好处？
<details><summary>答案</summary>
热路径函数集中放在一起 → icache 命中率高（执行时连续取指，局部性好）。冷路径函数分开远放 → 不占 icache 空间。GCC `-freorder-functions` 根据 PGO（Profile-Guided Optimization）自动分到这两个段。
</details>

5. 内核链接脚本中段填充值 `= 0xaa1a0000` 有什么作用？
<details><summary>答案</summary>
`0xaa1a0000` 是一条非法指令（ARM64 中会触发 Undefined 异常）。如果因为 bug 导致 CPU 跳到段间空洞或未填充区域，会立即触发异常而不是执行垃圾数据，方便调试定位问题。
</details>

## 参考与延伸

- 原书 §9.6
- [9.3 VMA vs LMA](03-vma-lma.md)
- [Ch14 MMU 虚拟地址](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
- Linux 内核源码：`arch/arm64/kernel/vmlinux.lds`
