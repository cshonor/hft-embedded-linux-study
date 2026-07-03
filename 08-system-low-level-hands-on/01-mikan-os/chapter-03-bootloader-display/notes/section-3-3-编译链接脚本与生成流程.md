# Ch 3 §3.3 典型编译生成流程与链接脚本

> **MikanOS** · 原书第 3 章

## 手写 OS 示例流程

```bash
# 1. 编译汇编、C/C++ 为 .o（freestanding = 不链接 libc）
clang++ -target x86_64-unknown-windows -ffreestanding -c kernel_main.cpp -o kernel_main.o
# 教学 32 位示例：
# gcc -m32 -ffreestanding -c boot.S -o boot.o
# gcc -m32 -ffreestanding -c kernel.c -o kernel.o

# 2. ld 链接 — 链接脚本决定各段加载地址
ld -m elf_x86_64 -T kernel.ld kernel_main.o ... -o kernel.elf

# 3. 可选：剥成纯二进制（无 ELF 头，裸烧录用）
objcopy -O binary kernel.elf kernel.bin
```

| 步骤 | 产出 | 谁消费 |
|------|------|--------|
| **编译 `-c`** | `.o` 重定位目标 | `ld` |
| **链接 `-T kernel.ld`** | **`kernel.elf`** | MikanLoader / QEMU / gdb |
| **objcopy -O binary** | **`kernel.bin`** | 极简 Bootloader（**无 ELF 元数据**） |

**MikanOS 实际构建** 用 **EDK II / LLVM** 封装 — 思想相同：**链接脚本 + 生成 ELF**。

---

## 链接脚本 `kernel.ld` / `kernel.lds` 决定什么

**链接脚本是 `kernel.elf` 的核心配置** — 告诉 `ld`：

| 配置项 | 后果 |
|--------|------|
| **`. = 0x100000`** 等 | 各段 **链接虚拟地址** — Loader 按 Program Header 搬到对应地址 |
| **`.text` / `.data` / `.bss` 顺序** | 哪些进 **同一 PT_LOAD 段** |
| **栈顶 `_stack_top`** | 内核栈位置 |
| **入口 `ENTRY(KernelMain)`** | 写入 ELF Header **`e_entry`** |

**Loader 无法加载的常见原因：**

- 链接地址落在 **UEFI 保留区 / MMIO**（与 [Ch2 memmap](../../chapter-02-edk2-memmap/) 冲突）
- 链接时 **没生成可执行 ELF**（只剩 `.o` 重定位文件）
- **32 位 ELF** 给 **64 位 Loader** 加载（架构不匹配）

---

## 最小 `kernel.ld` 示意（x86-64 风格，教学用）

```ld
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(KernelMain)

SECTIONS {
    . = 0x100000;          /* 内核加载基址 — 须在可用物理 RAM 内 */

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }

    /* 栈：在 BSS 之后留一片 */
    _stack_bottom = .;
    . = . + 0x10000;       /* 64 KiB 栈 */
    _stack_top = .;
}
```

> **注意：** 真实 MikanOS 仓库的 `kernel.ld` 以官方为准；此处仅说明 **「地址由脚本写死」**。

---

## BSS 与 `p_memsz > p_filesz`

| | `.data` | `.bss` |
|---|---------|--------|
| **磁盘上** | 有初值，占 `p_filesz` | **不占文件字节** |
| **内存里** | 载入 | Loader 或启动代码 **清零** `p_memsz - p_filesz` 区域 |

**QEMU 跑起来却变量乱飞** — 检查 **BSS 是否清零**。

---

← [3.2 ELF 双视图](./section-3-2-ELF三大结构与链接执行双视图.md) · 下一节 [3.4 常用命令](./section-3-4-readelf调试与常用命令.md)
