## Ch4 完整概述 · 汇编器规则与伪指令

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Assembler Rules and Directives · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **指挥汇编器** | 伪指令组织段、数据、常量、对齐、文字池 — **不进 CPU** |
| **双工具语法** | **Keil vs CCS** 对照；Linux 路线掌握 **GNU gas 等价** |
| **Ch3→Ch5 桥梁** | Ch3 见过 `AREA`/`LDR=`；本章系统化的 **文件语法** |

**前置：** [Ch3 完整概述](../../chapter-03-instruction-sets-v4t-v7m/notes/section-0-本章完整概述.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **行格式 · 常量** | §4.2 | [section-4-2-module-structure.md](./section-4-2-module-structure.md) |
| **寄存器别名** | §4.3 | [section-4-3-register-names.md](./section-4-3-register-names.md) |
| **伪指令大全** | §4.4 | [section-4-4-directives.md](./section-4-4-directives.md) |
| **宏** | §4.5 | [section-4-5-macros.md](./section-4-5-macros.md) |
| **汇编期运算** | §4.6 | [section-4-6-assembler-misc.md](./section-4-6-assembler-misc.md) |

---

### 三、Keil / CCS / GNU 一张表（语义）

| 语义 | Keil | CCS | GNU |
|------|------|-----|-----|
| 代码段 | `AREA … CODE` | `.text` | `.section .text` |
| 字节常量 | `DCB` | `.byte` | `.byte` / `.asciz` |
| 字常量 | `DCD` | `.word` | `.word` |
| 符号 | `EQU` | `.equ` | `.equ` |
| 对齐 | `ALIGN` | `.align` | `.align` / `.balign` |
| 文字池 | `LTORG` | （类似） | `.ltorg` |
| 入口 | `ENTRY` | C 运行时 | `.global` + 链接脚本 |

---

### 四、知识流（口述版）

```
行格式：标签第1列 · 指令缩进
        ↓
段(AREA/.section) 分 code/rodata/data/bss
        ↓
DCB/DCD 放表与字符串 · EQU 定义基址/掩码
        ↓
ALIGN 满足对齐 · LTORG 放 literal pool
        ↓
宏 inline 重复序列 · :SHL: 在汇编期算掩码
        ↓
Ch5：LDR/STR 访问这些地址与数据
```

---

### 五、与后续 / 支线

| Ch4 触点 | 展开 |
|----------|------|
| LTORG / 大常数 | **Ch6** |
| LDR 访存 | **Ch5** |
| 宏 vs BL | **Ch13** |
| MMIO 基址 EQU | **Ch16** · **Ch21 驱动** |
| 链接脚本 | [奔跑吧 Ch9](../../arm64-programming-practice/chapter-09-linker-scripts/) · [20 构建](../../../20-UBoot-Kernel-Build/) |

---

### 六、下一章

→ **[Ch5 加载、存储与寻址](../../chapter-05-loads-stores-addressing/)**（**精读**）
