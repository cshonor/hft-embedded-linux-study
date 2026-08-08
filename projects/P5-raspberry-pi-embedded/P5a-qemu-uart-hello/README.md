# P5a — QEMU 裸机 UART Hello World

> 在 QEMU 模拟的 ARM 机器上，不依赖任何 OS，直接写汇编点亮 UART 打印 "Hello"。
> **做法：项目驱动，[`10`](../../../10-arm-architecture/) 笔记当字典。**

---

## 最小预备

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [AArch64 基础](../../../10-arm-architecture/aarch64-practice/chapter-01-arm64-fundamentals/README.md) | ARM64 寄存器 x0-x30、SP、PC |
| [Pi5 实验路线](../../../10-arm-architecture/aarch64-practice/chapter-02-raspberry-pi-lab/notes/section-0-Pi5适配与实验路线.md) | 树莓派启动地址 0x80000 |
| [GNU 汇编器](../../../10-arm-architecture/aarch64-practice/chapter-08-gnu-assembler/README.md) | `.section` / `.global` / 指令语法 |
| [链接脚本](../../../10-arm-architecture/aarch64-practice/chapter-09-linker-scripts/README.md) | `. = 0x80000` 控制加载地址 |

---

## 项目目标

理解"裸机"——CPU 上电后第一条指令在哪、怎么跑到你的代码、怎么不用 printf 把字节送出串口。

## Phase 1：汇编写死一个字符（30 分钟）

### 做什么

最小裸机程序：往 UART 数据寄存器写一个字节 'H'。

### 代码骨架

```asm
// src/start.S
.section .text
.global _start

_start:
    // 树莓派 3 QEMU：UART0 数据寄存器在 0x3F215040
    // (Pi4/Pi5 地址不同，QEMU raspi3b 用 0x3F215040)
    ldr x0, =0x3F215040   // UART0 DR (Data Register)
    mov w1, #'H'           // 要发送的字符
    str w1, [x0]           // 写入 UART 数据寄存器

hang:
    w hang                 // 死循环
```

### 分步实现

1. **装交叉工具链**：`sudo apt install gcc-aarch64-linux-gnu`
2. **汇编**：`aarch64-linux-gnu-as -o start.o src/start.S`
3. **链接**：`aarch64-linux-gnu-ld -Ttext 0x80000 -o kernel8.elf start.o`
4. **提取二进制**：`aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img`
5. **QEMU 启动**：`qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial stdio -display none`
6. **看到 'H'** → 成功

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| UART 地址错 | 没输出 | Pi3=0x3F215040, Pi4=0xFE215040, QEMU 版本不同地址可能不同 |
| 没装 QEMU ARM 支持 | 命令找不到 | `sudo apt install qemu-system-arm` |
| 字符编码 | 输出乱码 | UART 用 `w`（32位写），不是 `b`（8位）|
| 链接地址错 | QEMU 不跑 | 树莓派启动地址是 0x80000，不是 0x0 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| AArch64 指令语法 | [ch03 load/store](../../../10-arm-architecture/aarch64-practice/chapter-03-a64-load-store/) |
| 汇编器语法 | [ch08 GNU assembler](../../../10-arm-architecture/aarch64-practice/chapter-08-gnu-assembler/) |

---

## Phase 2：封装 uart_puts，C 可调用（1 小时）

### 做什么

汇编设栈 → 跳 C main → C 里循环发字符串。

### 代码骨架

```asm
// src/start.S
.section .text
.global _start

_start:
    ldr x0, =_start        // 栈顶放在代码起点上方
    mov sp, x0
    bl main                // 跳转到 C 函数

hang:
    w hang
```

```c
// src/main.c
// UART 寄存器（Pi3 QEMU）
volatile unsigned int * const UART0_DR  = (unsigned int *)0x3F215040;
volatile unsigned int * const UART0_FR  = (unsigned int *)0x3F215018;  // Flag Register

void uart_putc(char c) {
    // 等 TX FIFO 非满（FR bit 5 = TXFF）
    while (*UART0_FR & (1 << 5)) { }
    *UART0_DR = c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');  // 串口要 \r\n
        uart_putc(*s++);
    }
}

void main(void) {
    uart_puts("Hello, bare metal!\n");
    for (;;) { }  // 不返回
}
```

### 分步实现

1. **汇编设栈**：`mov sp, x0`（栈向下生长，sp 指向代码起点）
2. **`bl main`**：跳到 C 函数
3. **`uart_putc`**：轮询 FR（Flag Register）的 TXFF 位，FIFO 满就等
4. **`uart_puts`**：逐字符发送，`\n` 前补 `\r`
5. **编译链接**：
   ```bash
   aarch64-linux-gnu-gcc -c -ffreestanding -O2 -o main.o src/main.c
   aarch64-linux-gnu-as -o start.o src/start.S
   aarch64-linux-gnu-ld -T linker.ld -o kernel8.elf start.o main.o
   aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img
   ```

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 忘了 `volatile` | 编译器优化掉寄存器读写 | UART 寄存器地址必须 volatile |
| 栈没设好 | C 函数调用就崩 | sp 必须指向有效内存 |
| `-ffreestanding` 没加 | 链接报错找不到 memset | 裸机没有标准库 |
| TX FIFO 没等满判断 | 丢字符 | 必须轮询 FR 寄存器 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| AArch64 调用约定 | [ch01 fundamentals](../../../10-arm-architecture/aarch64-practice/chapter-01-arm64-fundamentals/) |
| 异常等级 EL1/EL2 | [ch07 traps](../../../10-arm-architecture/aarch64-practice/chapter-07-a64-traps/) |

---

## Phase 3：链接脚本 + 完整 Hello（30 分钟）

### 代码骨架

```lds
/* src/linker.ld */
ENTRY(_start)

SECTIONS {
    . = 0x80000;            /* 树莓派加载地址 */

    .text : {
        *(.text)            /* 所有 .text 段 */
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }

    . = ALIGN(16);
    . = . + 0x10000;        /* 预留 64KB 栈空间 */
    stack_top = .;
}
```

### 分步实现

1. **写链接脚本**：指定 `. = 0x80000`，各段布局，预留栈空间
2. **在 start.S 里清 bss**：
   ```asm
   ldr x0, =__bss_start
   ldr x1, =__bss_end
   clear_bss:
       cmp x0, x1
       b.ge bss_done
       str xzr, [x0], #8
       b clear_bss
   bss_done:
   ```
3. **用 `stack_top` 设栈**：`ldr x0, =stack_top; mov sp, x0`
4. **完整流程**：start.S（设栈+清bss）→ main.c（UART 打印）→ linker.ld（地址布局）

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 链接脚本语法 | [ch09 linker scripts](../../../10-arm-architecture/aarch64-practice/chapter-09-linker-scripts/) |
| bss 段为什么要清零 | [ch09](../../../10-arm-architecture/aarch64-practice/chapter-09-linker-scripts/) |

---

## 测试验证

```bash
# 完整编译
aarch64-linux-gnu-gcc -c -ffreestanding -O2 -o main.o src/main.c
aarch64-linux-gnu-as -o start.o src/start.S
aarch64-linux-gnu-ld -T src/linker.ld -o kernel8.elf start.o main.o
aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img

# QEMU 运行
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial stdio -display none
# 预期输出：Hello, bare metal!
```

## 状态

⬜ 未开始 → 建议先装 `gcc-aarch64-linux-gnu` + `qemu-system-arm`，30 分钟跑出第一个字符。

← [P5 索引](../README.md) · [10 模块](../../../10-arm-architecture/)
