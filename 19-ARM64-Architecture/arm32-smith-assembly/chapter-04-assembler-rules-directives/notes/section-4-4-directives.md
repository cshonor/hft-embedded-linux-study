## §4.4 常用伪指令 — Keil / CCS / GNU

> **Ch 4 · 汇编器规则与伪指令** · [章导读](../README.md)

---

### 伪指令是什么

**不交给 CPU 执行** — 在 **汇编阶段** 指导汇编器：放哪个段、占多少字节、符号等于多少、何时插入文字池。

---

### 对照表（书中 Keil vs CCS + 本路线 GNU）

| 用途 | **Keil (ARM)** | **CCS (TI)** | **GNU gas**（Linux/WSL） |
|------|----------------|--------------|---------------------------|
| **代码/数据段** | `AREA name, CODE, READONLY` / `DATA` | `.sect "name"` · 默认 `.text`/`.data` | `.section .text` · `.data` · `.bss` · `.rodata` |
| **寄存器重命名** | `name RN r0` | `.asg r0, name` | `#define name r0`（C 预处理器）或注释 |
| **符号常量** | `symbol EQU expr` | `.set` / `.equ` | `.equ symbol, expr` 或 `#define` |
| **程序入口** | `ENTRY label` | `_c_int00` 等 C 运行时 | `.global _start` + 链接脚本 `ENTRY` |
| **定义字节/半字/字** | `DCB` / `DCW` / `DCD` | `.byte` / `.half` / `.word` / `.float` | `.byte` / `.hword` / `.word` / `.float` |
| **对齐** | `ALIGN n`（2^n 字节） | `.align n` | `.align n` 或 `.balign 4` |
| **保留零初始化空间** | `label SPACE n` | `.space` / `.bes` | `.space n` · `.comm` · `.lcomm` |
| **文字池** | `LTORG` | 类似 literal pool 机制 | `.ltorg` / 自动池 |
| **文件结束** | `END` | `.end` | （文件自然结束，无强制） |

---

### 分段 — 为何需要 AREA / .section

```
.text (CODE)     → 只读、可执行 — 指令
.rodata          → 只读 — 常量字符串、查找表
.data            → 可读写 — 已初始化全局变量
.bss / SPACE     → 可读写 — 零初始化变量、堆栈预留
```

**链接脚本**（[奔跑吧 Ch9](../../../arm64-programming-practice/chapter-09-linker-scripts/) · [20 构建](../../../../20-UBoot-Kernel-Build/)）决定各段 **最终 ROM/RAM 地址**。

---

### 数据定义示例

**Keil — 字符串与字常量：**

```asm
        AREA    Data1, DATA, READONLY
msg     DCB     "Hello", 0
val     DCD     0x40001000        ; MMIO 地址常数
```

**GNU：**

```gas
        .section .rodata
msg:    .asciz  "Hello"
        .equ    UART_BASE, 0x40001000
val:    .word   UART_BASE
```

**Ch5 预告：** `DCD` 定义的地址会被 `LDR rd, [pc, #offset]` 或 `LDR rd, =sym` 引用。

---

### ALIGN — 与 Ch2 对齐一致

| 要求 | 伪指令 |
|------|--------|
| 下一条指令/数据从 **4 字节边界** 开始 | Keil `ALIGN 2`（= 2²）· GNU `.align 2` |

**错对齐后果：** ARM7 上 **Data Abort**；M4 可能性能差或 fault（→ [Ch2 §2.2](../../chapter-02-programmers-model/notes/section-2-2-data-types.md)）。

---

### LTORG / 文字池

**问题：** `LDR r0, =0x12345678` 等 **大常数** 不能总嵌在指令里 → 汇编器在附近放 **literal pool**（常数表），PC 相对加载。

**LTORG：** 强制 **在此处** 生成文字池 — 防止 pool 离 `LDR` 太远超出 **±4KB** PC 相对范围（ARM 经典限制）。

→ 机制详解 **[Ch6 文字池](../../chapter-06-constants-literal-pools/)**。

---

### EQU / .set — 像 `#define`

```asm
UART0_BASE   EQU    0x40001000
BIT_TXEN     EQU    0x00000001
```

**驱动风格：** 基址 + 位掩码 — 配合 `LDR`/`STR` 写 MMIO（**Ch16**）。

---

### 可复述要点

1. **AREA/.section** 分代码与数据；**DCB/DCD/.word** 放常量；**SPACE/.bss** 留未初始化区。  
2. **ENTRY/.global** 声明入口；**END** 标记 Keil 文件尾。  
3. **LTORG** 解决大常数 PC 相对距离 — 与 Ch3 `LDR =` 一体。  
4. 本路线用 **GNU 列** 读 Linux 内核；Keil/CCS 懂语义即可。
