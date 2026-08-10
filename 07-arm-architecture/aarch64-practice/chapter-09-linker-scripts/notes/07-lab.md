# 9.7 实验要点

> 来源：§9.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 实验列表

| 实验 | 内容 | 关键命令 |
|------|------|---------|
| 9-1 | 编写简单链接脚本 | `gcc -T simple.ld` |
| 9-2 | VMA ≠ LMA 实验 | `readelf -l` 对比 |
| 9-3 | 分析 ELF 文件 | `readelf`/`nm`/`objdump` |
| 9-4 | --gc-sections 效果 | `size` 前后对比 |
| 9-5 | BenOS 链接脚本 | 完整裸机链接 |

---

## 实验 9-1：编写简单链接脚本

### 目标

编写最简单的链接脚本，理解位置计数器、段定义、ENTRY。

### 准备代码

```asm
/* start.S */
.section .text
.global _start
_start:
    mov x0, #42
    mov x8, #93        /* __NR_exit */
    svc #0            /* 系统调用退出 */
```

### 链接脚本

```ld
/* simple.ld */
ENTRY(_start)
SECTIONS
{
    . = 0x400000;         /* 起始地址 */
    .text : {
        *(.text)
    }
    . = ALIGN(8);
    .data : {
        *(.data)
    }
    .bss : {
        *(.bss)
    }
}
```

### 编译链接运行

```bash
# 编译
aarch64-linux-gnu-gcc -c start.S -o start.o

# 链接（用自定义脚本）
aarch64-linux-gnu-gcc -T simple.ld -nostdlib -o app.elf start.o

# 验证入口地址
readelf -h app.elf | grep "Entry"
# 期望: Entry point address: 0x400000

# 验证段布局
readelf -l app.elf

# 在 QEMU 中运行
qemu-aarch64 ./app.elf
echo $?                 # 期望: 42
```

### 观察要点

| 检查项 | 预期 |
|------|------|
| Entry point | 0x400000 |
| .text VirtAddr | 0x400000 |
| 段权限 | R E（只读可执行） |
| 无 .data/.bss | 正常（程序没用数据） |

---

## 实验 9-2：VMA ≠ LMA 实验

### 目标

创建 VMA ≠ LMA 的 ELF 文件，用 `readelf -l` 验证。

### 链接脚本

```ld
/* vma_lma.ld */
ENTRY(_start)
SECTIONS
{
    .text 0x80000 : AT(0x40000) {  /* VMA=0x80000, LMA=0x40000 */
        *(.text)
    }
    .data 0x90000 : AT(0x41000) {  /* VMA=0x90000, LMA=0x41000 */
        *(.data)
    }
    .bss 0x91000 : {
        *(.bss)
    }
}
```

### 操作步骤

```bash
# 编译链接
aarch64-linux-gnu-gcc -c start.S -o start.o
aarch64-linux-gnu-gcc -T vma_lma.ld -nostdlib -o vmlma.elf start.o

# 查看 Program Headers
readelf -l vmlma.elf

# 期望输出：
#   Type   Offset   VirtAddr   PhysAddr
#   LOAD   0x10000  0x00080000 0x00040000    ← VMA≠LMA
#   LOAD   0x11000  0x00090000 0x00041000    ← VMA≠LMA

# 查看地址关系
readelf -S vmlma.elf
# .text  Addr=0x80000  (VMA)
# .data  Addr=0x90000  (VMA)
```

### 观察要点

- VirtAddr 和 PhysAddr 不同 → VMA ≠ LMA
- 这种 ELF 不能直接运行（CPU 用 VMA 访问，但数据在 LMA）
- 实际内核启动代码需手动拷贝 .data 从 LMA 到 VMA

---

## 实验 9-3：分析 ELF 文件

### 目标

综合使用 readelf/nm/objdump/size 分析一个完整 ELF。

### 操作步骤

```bash
# 准备测试程序
cat > test.c << 'EOF'
#include <stdio.h>
int global_init = 42;      /* .data */
int global_uninit;         /* .bss */
static int local_static = 7;  /* .data (局部符号小写 d) */

void dead_function(void) {
    printf("never called\n");
}

int main(void) {
    printf("global_init = %d\n", global_init);
    return 0;
}
EOF

# 编译（带调试信息）
gcc -g -ffunction-sections -fdata-sections -o test.elf test.c

# === 1. 文件头 ===
readelf -h test.elf | head -20
# 关注: Entry point, Class(ELF64), Machine

# === 2. 段头表 ===
readelf -SW test.elf
# 关注: .text .data .bss .rodata .debug_info

# === 3. 程序头表 ===
readelf -l test.elf
# 关注: LOAD 段的 VMA/LMA/权限

# === 4. 符号表 ===
nm -n test.elf
# T main, D global_init, B global_uninit, d local_static

# === 5. 反汇编 ===
objdump -d test.elf | head -40
# 查看 main 的汇编代码

# === 6. 段大小 ===
size -A test.elf
# text + data + bss 总计
```

### 记录观察

| 检查项 | 观察值 |
|------|-------|
| Entry point | ? |
| .text VMA | ? |
| .data VMA | ? |
| .bss FileSiz vs MemSiz | ? |
| dead_function 是否在符号表中 | ✓（未 GC 时保留） |
| global_init 符号类型 | D |
| global_uninit 符号类型 | B |

---

## 实验 9-4：--gc-sections 效果

### 目标

对比 `--gc-sections` 前后的代码体积变化。

### 操作步骤

```bash
# 不带 GC 编译链接
gcc -ffunction-sections -fdata-sections -o test_nogc test.c
size -A test_nogc | head -10
# 记录 text 段大小

# 带 GC 编译链接
gcc -ffunction-sections -fdata-sections -Wl,--gc-sections -o test_gc test.c
size -A test_gc | head -10
# 对比 text 段大小

# 验证 dead_function 是否被删
nm test_nogc | grep dead_function    # 应该有
nm test_gc | grep dead_function      # 应该没有（被 GC 了）

# 查看 map 文件
gcc -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,-Map=test.map -o test_gc test.c
grep "discarded" test.map
# 应该看到 dead_function 被标记为 discarded
```

### 记录对比

| 项目 | 不带 GC | 带 GC | 差异 |
|------|---------|-------|------|
| text 段大小 | ? | ? | 减少 ? |
| dead_function 存在 | ✓ | ✗ | 删除 |
| 总文件大小 | ? | ? | 减少 ? |

---

## 实验 9-5：BenOS 链接脚本

### 目标

编写一个完整的裸机链接脚本，理解 MEMORY 块、多段布局、符号导出。

### 链接脚本

```ld
/* benos.ld */
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000, LENGTH = 256M
}

SECTIONS
{
    . = 0x80000;

    /* 代码段 */
    .text : {
        _text_start = .;
        *(.text.boot)        /* 启动专用代码放最前 */
        *(.text)
        _text_end = .;
    } > RAM

    /* 只读数据 */
    . = ALIGN(4096);
    .rodata : {
        _rodata_start = .;
        *(.rodata)
        _rodata_end = .;
    } > RAM

    /* 已初始化数据 */
    . = ALIGN(4096);
    .data : {
        _data_start = .;
        *(.data)
        _data_end = .;
    } > RAM

    /* 未初始化数据 */
    . = ALIGN(8);
    .bss : {
        _bss_start = .;
        *(.bss)
        *(COMMON)
        _bss_end = .;
    } > RAM

    /* 栈（从内存顶部向下生长） */
    . = ALIGN(16);
    . = ORIGIN(RAM) + LENGTH(RAM) - 0x1000;
    _stack_top = .;

    /* 断言检查 */
    ASSERT(_bss_end <= ORIGIN(RAM) + LENGTH(RAM) - 0x1000,
           "ERROR: Image overflows RAM!")
}
```

### 操作步骤

```bash
# 编译
aarch64-linux-gnu-gcc -c start.S -o start.o
aarch64-linux-gnu-gcc -c main.c -o main.o

# 链接
aarch64-linux-gnu-gcc -T benos.ld -nostdlib -o benos.elf start.o main.o

# 验证
readelf -l benos.elf
nm -n benos.elf | grep -E "(_text|_data|_bss|_stack)"

# 在 QEMU 中运行
qemu-system-aarch64 -M virt -cpu cortex-a57 \
    -kernel benos.elf -nographic -m 256M
```

### 观察要点

| 检查项 | 预期 |
|------|------|
| Entry point | 0x80000 |
| _text_start | 0x80000 |
| _stack_top | 0x80000 + 256M - 0x1000 |
| 段权限 | .text R E, .data RW |
| ASSERT | 无 overflow 错误 |

## 自测题

1. 如何验证 VMA ≠ LMA？
<details><summary>答案</summary>
`readelf -l 文件名` 查看 Program Headers。比较 VirtAddr 列（VMA）和 PhysAddr 列（LMA）。两者不同 → VMA≠LMA。也可以用 `objdump -h` 看 VMA 和 LMA 列。
</details>

2. `--gc-sections` 前后 size 输出变化说明了什么？
<details><summary>答案</summary>
text 段变小 → 未引用的函数被删除（死代码消除）。需要 `-ffunction-sections` 让每个函数独立成段才能精确删除。`-Map` 文件中可以看到被丢弃的段标记为 "discarded"。
</details>

3. 为什么裸机链接脚本要在 .bss 之后定义 `_stack_top`？
<details><summary>答案</summary>
栈从内存顶部向下生长。_stack_top 指向 RAM 顶部（如 ORIGIN+LENGTH-0x1000），留给栈向下生长的空间。.bss 之后到 _stack_top 之间的空间就是栈空间。如果程序用了太多 RAM，.bss 可能和栈重叠 → ASSERT 检查会报错。
</details>

4. `KEEP()` 在 BenOS 链接脚本中应该怎么用？
<details><summary>答案</summary>
对启动代码段用 `KEEP(*(.text.boot))`，防止 `--gc-sections` 误删。因为 _start 在 .text.boot 中，GC 从 ENTRY 可达性分析应能保留它，但如果有复杂的初始化函数间引用链，用 KEEP 更保险。
</details>

5. 如何在 QEMU 中测试自制链接脚本的程序？
<details><summary>答案</summary>
（1）`qemu-system-aarch64 -M virt -cpu cortex-a57 -kernel benos.elf -nographic`（2）QEMU 的 `-kernel` 会跳到 ELF 的入口地址（3）需配合串口输出（`-nographic` 用串口作为 stdout）（4）如果程序用了 UART MMIO 地址（0x09000000 for virt machine）就能看到输出。
</details>

## 参考与延伸

- 原书 §9.7
- [9.5 分析链接结果](05-analyze-output.md)
- [9.8 易错点清单](08-pitfalls.md)
